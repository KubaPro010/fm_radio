#include "rds.h"
#include <string.h>
#include <map>
#include <algorithm>

#include <utils/flog.h>


namespace rds {
	std::map<uint16_t, BlockType> SYNDROMES = {
		{ 0b1111011000, BLOCK_TYPE_A  },
		{ 0b1111010100, BLOCK_TYPE_B  },
		{ 0b1001011100, BLOCK_TYPE_C  },
		{ 0b1111001100, BLOCK_TYPE_CP },
		{ 0b1001011000, BLOCK_TYPE_D  }
	};

	std::map<BlockType, uint16_t> OFFSETS = {
		{ BLOCK_TYPE_A,  0b0011111100 },
		{ BLOCK_TYPE_B,  0b0110011000 },
		{ BLOCK_TYPE_C,  0b0101101000 },
		{ BLOCK_TYPE_CP, 0b1101010000 },
		{ BLOCK_TYPE_D,  0b0110110100 }
	};

	std::map<uint16_t, const char*> THREE_LETTER_CALLS = {
		{ 0x99A5, "KBW" },
		{ 0x99A6, "KCY" },
		{ 0x9990, "KDB" },
		{ 0x99A7, "KDF" },
		{ 0x9950, "KEX" },
		{ 0x9951, "KFH" },
		{ 0x9952, "KFI" },
		{ 0x9953, "KGA" },
		{ 0x9991, "KGB" },
		{ 0x9954, "KGO" },
		{ 0x9955, "KGU" },
		{ 0x9956, "KGW" },
		{ 0x9957, "KGY" },
		{ 0x99AA, "KHQ" },
		{ 0x9958, "KID" },
		{ 0x9959, "KIT" },
		{ 0x995A, "KJR" },
		{ 0x995B, "KLO" },
		{ 0x995C, "KLZ" },
		{ 0x995D, "KMA" },
		{ 0x995E, "KMJ" },
		{ 0x995F, "KNX" },
		{ 0x9960, "KOA" },
		{ 0x99AB, "KOB" },
		{ 0x9992, "KOY" },
		{ 0x9993, "KPQ" },
		{ 0x9964, "KQV" },
		{ 0x9994, "KSD" },
		{ 0x9965, "KSL" },
		{ 0x9966, "KUJ" },
		{ 0x9995, "KUT" },
		{ 0x9967, "KVI" },
		{ 0x9968, "KWG" },
		{ 0x9996, "KXL" },
		{ 0x9997, "KXO" },
		{ 0x996B, "KYW" },
		{ 0x9999, "WBT" },
		{ 0x996D, "WBZ" },
		{ 0x996E, "WDZ" },
		{ 0x996F, "WEW" },
		{ 0x999A, "WGH" },
		{ 0x9971, "WGL" },
		{ 0x9972, "WGN" },
		{ 0x9973, "WGR" },
		{ 0x999B, "WGY" },
		{ 0x9975, "WHA" },
		{ 0x9976, "WHB" },
		{ 0x9977, "WHK" },
		{ 0x9978, "WHO" },
		{ 0x999C, "WHP" },
		{ 0x999D, "WIL" },
		{ 0x997A, "WIP" },
		{ 0x99B3, "WIS" },
		{ 0x997B, "WJR" },
		{ 0x99B4, "WJW" },
		{ 0x99B5, "WJZ" },
		{ 0x997C, "WKY" },
		{ 0x997D, "WLS" },
		{ 0x997E, "WLW" },
		{ 0x999E, "WMC" },
		{ 0x999F, "WMT" },
		{ 0x9981, "WOC" },
		{ 0x99A0, "WOI" },
		{ 0x9983, "WOL" },
		{ 0x9984, "WOR" },
		{ 0x99A1, "WOW" },
		{ 0x99B9, "WRC" },
		{ 0x99A2, "WRR" },
		{ 0x99A3, "WSB" },
		{ 0x99A4, "WSM" },
		{ 0x9988, "WWJ" },
		{ 0x9989, "WWL" }
	};

	std::map<uint16_t, const char*> NAT_LOC_LINKED_STATIONS = {
		{ 0xB01, "NPR-1" },
		{ 0xB02, "CBC - Radio One" },
		{ 0xB03, "CBC - Radio Two" },
		{ 0xB04, "Radio-Canada - Première Chaîne" },
		{ 0xB05, "Radio-Canada - Espace Musique" },
		{ 0xB06, "CBC" },
		{ 0xB07, "CBC" },
		{ 0xB08, "CBC" },
		{ 0xB09, "CBC" },
		{ 0xB0A, "NPR-2" },
		{ 0xB0B, "NPR-3" },
		{ 0xB0C, "NPR-4" },
		{ 0xB0D, "NPR-5" },
		{ 0xB0E, "NPR-6" }
	};

	const uint16_t LFSR_POLY = 0b0110111001;
	const uint16_t IN_POLY   = 0b1100011011;

	const int BLOCK_LEN = 26;
	const int DATA_LEN = 16;
	const int POLY_LEN = 10;

	void Decoder::process(uint8_t* symbols, int count) {
		for (int i = 0; i < count; i++) {
			// Shift in the bit
			shiftReg = ((shiftReg << 1) & 0x3FFFFFF) | (symbols[i] & 1);

			// Skip if we need to shift in new data
			if (--skip > 0) continue;

			// Calculate the syndrome and update sync status
			uint16_t syn = calcSyndrome(shiftReg);
			auto synIt = SYNDROMES.find(syn);
			bool knownSyndrome = synIt != SYNDROMES.end();
			sync = std::clamp<int>(knownSyndrome ? ++sync : --sync, 0, 4);

			// If we're still no longer in sync, try to resync
			if (!sync) continue;

			// Figure out which block we've got
			BlockType type;
			if (knownSyndrome) type = SYNDROMES[syn];
			else type = (BlockType)((lastType + 1) % _BLOCK_TYPE_COUNT);

			// Save block while correcting errors
			blocks[type] = correctErrors(shiftReg, type, blockAvail[type]);

			// If block type is A, decode it directly, otherwise, update continous count
			if (type == BLOCK_TYPE_A) decodeBlockA();
			else if (type == BLOCK_TYPE_B)  contGroup = 1;
			else if ((type == BLOCK_TYPE_C || type == BLOCK_TYPE_CP) && lastType == BLOCK_TYPE_B) contGroup++;
			else if (type == BLOCK_TYPE_D && (lastType == BLOCK_TYPE_C || lastType == BLOCK_TYPE_CP)) contGroup++;
			else {
				// If block B is available, decode it alone.
				if (contGroup == 1) decodeBlockB();
				contGroup = 0;
			}

			// If we've got an entire group, process it
			if (contGroup >= 3) {
				contGroup = 0;
				decodeGroup();
			}

			// Remember the last block type and skip to new block
			lastType = type;
			skip = BLOCK_LEN;
		}
	}

	uint16_t Decoder::calcSyndrome(uint32_t block) {
		uint16_t syn = 0;

		// Calculate the syndrome using a LFSR
		for (int i = BLOCK_LEN - 1; i >= 0; i--) {
			// Shift the syndrome and keep the output
			uint8_t outBit = (syn >> (POLY_LEN - 1)) & 1;
			syn = (syn << 1) & 0b1111111111;

			// Apply LFSR polynomial
			syn ^= LFSR_POLY * outBit;

			// Apply input polynomial.
			syn ^= IN_POLY * ((block >> i) & 1);
		}

		return syn;
	}

	uint32_t Decoder::correctErrors(uint32_t block, BlockType type, bool& recovered) {
		// Subtract the offset from block
		block ^= (uint32_t)OFFSETS[type];
		uint32_t out = block;

		// Calculate syndrome of corrected block
		uint16_t syn = calcSyndrome(block);

		// Use the syndrome register to do error correction if errors are present
		uint8_t errorFound = 0;
		if (syn) {
			for (int i = DATA_LEN - 1; i >= 0; i--) {
				// Check if the 5 leftmost bits are all zero
				errorFound |= !(syn & 0b11111);

				// Write output
				uint8_t outBit = (syn >> (POLY_LEN - 1)) & 1;
				out ^= (errorFound & outBit) << (i + POLY_LEN);

				// Shift syndrome
				syn = (syn << 1) & 0b1111111111;
				syn ^= LFSR_POLY * outBit * !errorFound;
			}
		}
		recovered = !(syn & 0b11111);

		return out;
	}

	unsigned int Decoder::getMJDYear(double mjd) {
		unsigned int year  = int((mjd - 15078.2) / 365.25);
		unsigned int month = int((mjd - 14956.1 - int(year * 365.25)) / 30.6001);
		bool K = ((month == 14) || (month == 15)) ? 1 : 0;
		year += K;
		year += 1900;
		return year % 9999;
	}

	unsigned int Decoder::getMJDMonth(double mjd) {
		unsigned int year  = int((mjd - 15078.2) / 365.25);
		unsigned int month = int((mjd - 14956.1 - int(year * 365.25)) / 30.6001);
		bool K = ((month == 14) || (month == 15)) ? 1 : 0;
		month -= 1 + K * 12;
		return month % 13;
	}

	unsigned int Decoder::getMJDDay(double mjd) {
		unsigned int year  = int((mjd - 15078.2) / 365.25);
		unsigned int month = int((mjd - 14956.1 - int(year * 365.25)) / 30.6001);
		unsigned int day   = mjd - 14956 - int(year * 365.25) - int(month * 30.6001);
		return day % 32;
	}

	void Decoder::decodeBlockA() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(blockAMtx);

		// If it didn't decode properly return
		if (!blockAvail[BLOCK_TYPE_A]) { return; }

		// Decode PI code
		piCode = (blocks[BLOCK_TYPE_A] >> 10) & 0xFFFF; /* bitwise by ten because we still have the offset here */
		programCoverage = (AreaCoverage)((blocks[BLOCK_TYPE_A] >> 18) & 0xF);
		callsign = decodeCallsign(piCode);

		// Update timeout
		blockALastUpdate = std::chrono::high_resolution_clock::now();;
	}

	void Decoder::decodeBlockB() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(blockBMtx);

		// If it didn't decode properly return (TODO: Make sure this is not needed)
		if (!blockAvail[BLOCK_TYPE_B]) { return; }

		// Decode group type and version
		groupType = (blocks[BLOCK_TYPE_B] >> 22) & 0xF;
		groupVer = (GroupVersion)((blocks[BLOCK_TYPE_B] >> 21) & 1);

		// Decode traffic program and program type
		trafficProgram = (blocks[BLOCK_TYPE_B] >> 20) & 1;
		programType = (ProgramType)((blocks[BLOCK_TYPE_B] >> 15) & 0x1F);

		// Update timeout
		blockBLastUpdate = std::chrono::high_resolution_clock::now();
	}

	void Decoder::decodeAlternativeFrequencies() {
		if(alternativeFrequency) {
			uint8_t af0 = (alternativeFrequency >> 8) & 0xff;
			uint8_t af1 = alternativeFrequency & 0xff;

			if (af0 >= ALTERNATIVE_FREQUENCY_SPECIAL_CODES_AF_COUNT_BASE && af0 <= ALTERNATIVE_FREQUENCY_SPECIAL_CODES_AF_COUNT_BASE+25) {
				// First message is 224+n with n being the number of AFs, when we receive that we set the afCount and reset the afState in order to be in sync with the encoder
				afCount = af0 - ALTERNATIVE_FREQUENCY_SPECIAL_CODES_AF_COUNT_BASE;
				if(afCount > 25) {
					// If the count is greater than 25, we set it to 25
					afCount = 25;
				}
				if(af1 == ALTERNATIVE_FREQUENCY_SPECIAL_CODES_LFMF_FOLLOWS) {
					// If the first AF is 250, we set the afLfMfIncoming to 1, because af0 here is the len, so if af1 is lfmf incoming then it is logical that the lfmf is in the next message
					afLfMfIncoming = 1;
					return;
				}
				afState = 0;
			}

			if(afState == 0) {
				// Decode the first AF, which is next to the count
				if(afCount != 0) {
					if(af1 == ALTERNATIVE_FREQUENCY_SPECIAL_CODES_LFMF_FOLLOWS) afLfMfIncoming = 1;
					else if(af1 != ALTERNATIVE_FREQUENCY_SPECIAL_CODES_FILLER) {
						if(afLfMfIncoming) {
							afLfMfIncoming = 0;
							if (af1 <= 15) afs[0] = (af1 - 1) * 9 + 153;
							else afs[0] = 9 * (af1 - 16) + 531;
						} else {
							afs[0] = (af1 + 875) * 100;
						}
					}
					afState++;
				}
				return;
			} else {
				// Decode rest of the AFs
				if(afLfMfIncoming) {
					afLfMfIncoming = 0;
					if (af0 <= 15) afs[afState] = (af0 - 1) * 9 + 153;
					else afs[afState] = 9 * (af0 - 16) + 531;
					afState++;

					if(af1 == ALTERNATIVE_FREQUENCY_SPECIAL_CODES_LFMF_FOLLOWS) afLfMfIncoming = 1;
					else if(af1 != 205 && afState < 23) {
						afs[afState] = (af1 + 875) * 100;
						afState++;
					}
				} else if(af0 == ALTERNATIVE_FREQUENCY_SPECIAL_CODES_LFMF_FOLLOWS) {
					if (af1 <= 15) afs[afState] = (af1 - 1) * 9 + 153;
					else afs[afState] = 9 * (af1 - 16) + 531;
					afState++;
				} else {
					afs[afState] = (af0 + 875) * 100;
					afState++;

					if(af1 == ALTERNATIVE_FREQUENCY_SPECIAL_CODES_LFMF_FOLLOWS) afLfMfIncoming = 1;
					else if(af1 != ALTERNATIVE_FREQUENCY_SPECIAL_CODES_FILLER && afState < 23) {
						afs[afState] = (af1 + 875) * 100;
						afState++;
					}
				}
			}

			if(afState > 25) {
				afState = 0;
				return;
			}
		}
	}

	void Decoder::decodeGroup0() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(group0Mtx);

		// Decode Block B data
		trafficAnnouncement = (blocks[BLOCK_TYPE_B] >> 14) & 1;
		uint8_t diBit = (blocks[BLOCK_TYPE_B] >> 12) & 1;
		uint8_t segment = ((blocks[BLOCK_TYPE_B] >> 10) & 0b11);
		uint8_t diBitPlacement = 3 - segment;
		uint8_t psSegment = segment * 2;

		// Decode Block C data
		if (groupVer == GROUP_VER_A && blockAvail[BLOCK_TYPE_C]) {
			alternativeFrequency = (blocks[BLOCK_TYPE_C] >> 10) & 0xFFFF;
			decodeAlternativeFrequencies();
		}

		// Write DI bit to the decoder identification
		decoderIdent &= ~(1 << diBitPlacement);
		decoderIdent |= (diBit << diBitPlacement);

		// Write chars at segment the PSName
		if (blockAvail[BLOCK_TYPE_D]) {
			ps[psSegment + 0] = (blocks[BLOCK_TYPE_D] >> 18) & 0xFF;
			ps[psSegment + 1] = (blocks[BLOCK_TYPE_D] >> 10) & 0xFF;
		}

		// Update timeout
		group0LastUpdate = std::chrono::high_resolution_clock::now();
	}

	void Decoder::decodeGroup1A() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(group1Mtx);

		if (blockAvail[BLOCK_TYPE_C]) {
			/* Decode the Slow Labeling Codes, these are only in group 1A */
			uint8_t variant_code = (blocks[BLOCK_TYPE_C] >> 22) & 0b111;

			if(variant_code == 0) {
				/* ECC */
				ecc = (blocks[BLOCK_TYPE_C] >> 10) & 0xFF; /* ECC is a single byte, 8 bits */
				eccLastUpdate = std::chrono::high_resolution_clock::now();
			}
		}

		// Update timeout
		group1LastUpdate = std::chrono::high_resolution_clock::now();
	}

	void Decoder::decodeGroup2() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(group2Mtx);

		// Get char segment and write chars in the Radiotext
		bool rtAB = (blocks[BLOCK_TYPE_B] >> 14) & 1;
		uint8_t segment = (blocks[BLOCK_TYPE_B] >> 10) & 0xF;

		// Clear text field if the A/B flag changed
		if (rtAB != lastRTAB) {
			radioText = "                                                                ";
		}
		lastRTAB = rtAB;

		// Write char at segment in Radiotext
		if (groupVer == GROUP_VER_A) {
			uint8_t rtSegment = segment * 4;
			if (blockAvail[BLOCK_TYPE_C]) {
				radioText[rtSegment + 0] = (blocks[BLOCK_TYPE_C] >> 18) & 0xFF;
				radioText[rtSegment + 1] = (blocks[BLOCK_TYPE_C] >> 10) & 0xFF;
			}
			if (blockAvail[BLOCK_TYPE_D]) {
				radioText[rtSegment + 2] = (blocks[BLOCK_TYPE_D] >> 18) & 0xFF;
				radioText[rtSegment + 3] = (blocks[BLOCK_TYPE_D] >> 10) & 0xFF;
			}
		}
		else {
			uint8_t rtSegment = segment * 2;
			if (blockAvail[BLOCK_TYPE_D]) {
				radioText[rtSegment] = (blocks[BLOCK_TYPE_D] >> 18) & 0xFF;
				radioText[rtSegment + 1] = (blocks[BLOCK_TYPE_D] >> 10) & 0xFF;
			}
		}

		// Update timeout
		group2LastUpdate = std::chrono::high_resolution_clock::now();
	}

	void Decoder::decodeGroup3A() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(group3AMtx);

		GroupVersion groupVer_oda = (GroupVersion)((blocks[BLOCK_TYPE_B] >> 10) & 1);
		uint8_t groupType_oda = (blocks[BLOCK_TYPE_B] >> 11) & 0xF;
		uint16_t aid = blocks[BLOCK_TYPE_D] >> 10;

		if(aid == 0) {
			// If the AID is 0, we don't need to do anything
			return;
		}

		for(int i = 0; i < oda_aid_count; i++) {
			if(odas_aid[i].AID == aid) {
				// If we already have this AID, just update the group type
				odas_aid[i].GroupType = groupType_oda;
				odas_aid[i].GroupVer = groupVer_oda;
				return;
			}
		}

		// If we don't have this AID, add it to the list
		if(oda_aid_count < 8) {
			odas_aid[oda_aid_count].AID = aid;
			odas_aid[oda_aid_count].GroupType = groupType_oda;
			odas_aid[oda_aid_count].GroupVer = groupVer_oda;
			oda_aid_count++;
		}
		else {
			// If we don't have space, remove the oldest AID and add the new one
			odas_aid[0].AID = aid;
			odas_aid[0].GroupType = groupType_oda;
			odas_aid[0].GroupVer = groupVer_oda;
		}

		switch(aid) {
			case 0x6552:
				// ERT
				decodeDataERT();
				break;
		}
	}

	void Decoder::decodeGroup4A() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(group4AMtx);

		if(blockAvail[BLOCK_TYPE_C]) {
			// MJD is in the last bits of block b and whole block c
			clock_mjd = (((blocks[BLOCK_TYPE_B] >> 10) & 0x03) << 15) | (((blocks[BLOCK_TYPE_C] >> 10) >> 1) & 0x7fff);
		}
		if(blockAvail[BLOCK_TYPE_C] && blockAvail[BLOCK_TYPE_D]) {
			// Hour is somewhat in block C but rest are in D
			clock_hour = ((blocks[BLOCK_TYPE_C] >> 10) & 1);
			clock_hour <<= 4;
			clock_hour |= blocks[BLOCK_TYPE_D] >> 22;

			clock_minute = ((blocks[BLOCK_TYPE_D] >> 16) & 0x3f);

			clock_offset_sense = ((blocks[BLOCK_TYPE_D] >> 15) & 1);

			clock_offset = ((blocks[BLOCK_TYPE_D] >> 10) & 0x1f);
		}
	}

	void Decoder::decodeGroup10A() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(group10AMtx);

		// Check if the text needs to be cleared
		bool ab = (blocks[BLOCK_TYPE_B] >> 14) & 1;
		if (ab != lastPTYNAB) {
			programTypeName = "        ";
		}
		lastPTYNAB = ab;

		// Decode segment address
		bool seg = (blocks[BLOCK_TYPE_B] >> 10) & 1;

		// Save text depending on address
		if (seg) {
			if (blockAvail[BLOCK_TYPE_C]) {
				programTypeName[4] = (blocks[BLOCK_TYPE_C] >> 18) & 0xFF;
				programTypeName[5] = (blocks[BLOCK_TYPE_C] >> 10) & 0xFF;
			}
			if (blockAvail[BLOCK_TYPE_D]) {
				programTypeName[6] = (blocks[BLOCK_TYPE_D] >> 18) & 0xFF;
				programTypeName[7] = (blocks[BLOCK_TYPE_D] >> 10) & 0xFF;
			}
		}
		else {
			if (blockAvail[BLOCK_TYPE_C]) {
				programTypeName[0] = (blocks[BLOCK_TYPE_C] >> 18) & 0xFF;
				programTypeName[1] = (blocks[BLOCK_TYPE_C] >> 10) & 0xFF;
			}
			if (blockAvail[BLOCK_TYPE_D]) {
				programTypeName[2] = (blocks[BLOCK_TYPE_D] >> 18) & 0xFF;
				programTypeName[3] = (blocks[BLOCK_TYPE_D] >> 10) & 0xFF;
			}
		}

		// Update timeout
		group10ALastUpdate = std::chrono::high_resolution_clock::now();
	}

	void Decoder::decodeGroup15A() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(group15AMtx);

		if (blockAvail[BLOCK_TYPE_C] && blockAvail[BLOCK_TYPE_D]) {
			/* Decode Long PS */

			uint8_t segment = (blocks[BLOCK_TYPE_B] >> 10) & 0b111;
			uint8_t lpsSegment = segment * 4;
			longPS[lpsSegment + 0] = (blocks[BLOCK_TYPE_C] >> 18) & 0xFF;
			longPS[lpsSegment + 1] = (blocks[BLOCK_TYPE_C] >> 10) & 0xFF;
			longPS[lpsSegment + 2] = (blocks[BLOCK_TYPE_D] >> 18) & 0xFF;
			longPS[lpsSegment + 3] = (blocks[BLOCK_TYPE_D] >> 10) & 0xFF;
		}

		// Update timeout
		group15ALastUpdate = std::chrono::high_resolution_clock::now();
	}

	void Decoder::decodeGroup15B() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(group0Mtx);

		bool ta_c = 0;
		bool di_c = 0;
		uint8_t segment_c = 0;
		bool ta_d = 0;
		bool di_d = 0;
		uint8_t segment_d = 0;

		if(blockAvail[BLOCK_TYPE_C]) {
			segment_c = (blocks[BLOCK_TYPE_C] >> 10) & 0b11;
			ta_c = (blocks[BLOCK_TYPE_C] >> 14) & 1;
			di_c = (blocks[BLOCK_TYPE_C] >> 12) & 1;
		}
		if(blockAvail[BLOCK_TYPE_D]) {
			segment_d = (blocks[BLOCK_TYPE_D] >> 10) & 0b11;
			ta_d = (blocks[BLOCK_TYPE_D] >> 14) & 1;
			di_d = (blocks[BLOCK_TYPE_D] >> 12) & 1;
		}
		if (blockAvail[BLOCK_TYPE_C] && blockAvail[BLOCK_TYPE_D] && (segment_c != segment_d || ta_c != ta_d || di_c != di_d)) return;
		trafficAnnouncement = ta_c;

		uint8_t diBit = di_c;
		uint8_t diBitPlacement = 3 - segment_c;
		decoderIdent &= ~(1 << diBitPlacement);
		decoderIdent |= (diBit << diBitPlacement);
	}

	void Decoder::decodeGroupRTP() {
		std::lock_guard<std::mutex> lck(rtpMtx);
	
		uint8_t b_lower = (blocks[BLOCK_TYPE_B] >> 10) & 0xFF;
	
		rtp_item_toggle = (b_lower >> 4) & 0x01;
		rtp_item_running = (b_lower >> 3) & 0x01;
	
		uint8_t type1_upper = b_lower & 0x07;
	
		if (blockAvail[BLOCK_TYPE_C]) {
			uint16_t c = blocks[BLOCK_TYPE_C] >> 10;
			uint8_t type1_lower = (c >> 13) & 0x07;
			rtp_content_type_1 = (type1_upper << 3) | type1_lower;
			rtp_content_type_1_start = (c >> 7) & 0x3F;
			rtp_content_type_1_len   = (c >> 1) & 0x3F;			
			if (rtp_content_type_1_start > 63) rtp_content_type_1_start = 0;
			if (rtp_content_type_1_len > 64 || rtp_content_type_1_start + rtp_content_type_1_len > 64)
				rtp_content_type_1_len = 64 - rtp_content_type_1_start;			
			uint8_t type2_upper = c & 0x01;
	
			if (blockAvail[BLOCK_TYPE_D]) {
				uint16_t d = blocks[BLOCK_TYPE_D] >> 10;
				uint8_t type2_lower = (d >> 11) & 0x1F;
				rtp_content_type_2 = (type2_upper << 5) | type2_lower;
				rtp_content_type_2_start = (d >> 5) & 0x3F;
				rtp_content_type_2_len = d & 0x1F;
				if (rtp_content_type_2_start > 63) rtp_content_type_2_start = 0;
				if (rtp_content_type_2_len > 64 || rtp_content_type_2_start + rtp_content_type_2_len > 64)
					rtp_content_type_2_len = 64 - rtp_content_type_2_start;
				
			}
		} else return;
	
		rtpLastUpdate = std::chrono::high_resolution_clock::now();
	}	

	void Decoder::decodeGroupERT() {
		// Acquire lock
		std::lock_guard<std::mutex> lck(ertMtx);

		// Get char segment and write chars in the Radiotext
		// No AB flag in this group
		uint8_t segment = (blocks[BLOCK_TYPE_B] >> 10) & 0x1F;
	
		// Write char at segment in Radiotext
		uint8_t ertSegment = segment * 4;
		if(ert_direction) ertSegment = 128-ertSegment;
		if (blockAvail[BLOCK_TYPE_C]) {
			ert[ertSegment + 0] = (blocks[BLOCK_TYPE_C] >> 18) & 0xFF;
			ert[ertSegment + 1] = (blocks[BLOCK_TYPE_C] >> 10) & 0xFF;

			// Clear ert if \r is present
			if (ert[ertSegment + 0] == 0x0D) {
				for (size_t i = ertSegment; i < 128; ++i) ert[i] = ' ';
			} else if(ert[ertSegment + 1] == 0x0D) {
				for (size_t i = ertSegment + 1; i < 128; ++i) ert[i] = ' ';
			}
		}
		if (blockAvail[BLOCK_TYPE_D]) {
			ert[ertSegment + 2] = (blocks[BLOCK_TYPE_D] >> 18) & 0xFF;
			ert[ertSegment + 3] = (blocks[BLOCK_TYPE_D] >> 10) & 0xFF;

			// Clear ert if \r is present
			if (ert[ertSegment + 2] == 0x0D) {
				for (size_t i = ertSegment + 2; i < 128; ++i) ert[i] = ' ';
			} else if(ert[ertSegment + 3] == 0x0D) {
				for (size_t i = ertSegment + 3; i < 128; ++i) ert[i] = ' ';
			}
		}

		// Update timeout
		ertLastUpdate = std::chrono::high_resolution_clock::now();
	}

	void Decoder::decodeDataERT() {
		std::lock_guard<std::mutex> lck(ertMtx);

		ert_ucs2 = (blocks[BLOCK_TYPE_C] >> 10) & 1;
		ert_direction = (blocks[BLOCK_TYPE_C] >> 11) & 1;
	}

	void Decoder::decodeGroupODA() {
		uint16_t aid = 0;

		// First check if we know what this is
		for(int i = 0; i < oda_aid_count; i++) {
			if(odas_aid[i].AID == 0) {
				// If the AID is 0, we don't need to do anything, we have no clue what this is
				return;
			}
			if(odas_aid[i].GroupType == groupType && odas_aid[i].GroupVer == groupVer) {
				// Found it!
				aid = odas_aid[i].AID;
				break;
			}
		}

		if(aid == 0) {
			// If we don't know what this is, we don't need to do anything
			return;
		}

		switch(aid) {
			case 0x4BD7:
				// RTP
				if(groupVer == GROUP_VER_A) decodeGroupRTP();
				break;
			case 0x6552:
				// ERT
				if(groupVer == GROUP_VER_A) decodeGroupERT();
				break;
			default:
				// Unknown AID
				break;
		}
	}

	void Decoder::decodeGroup() {
		// Make sure blocks B is available
		if (!blockAvail[BLOCK_TYPE_B]) { return; }

		// Decode block B
		decodeBlockB();

		// Decode depending on group type
		switch (groupType) {
		case 0:
			// PS, AF
			decodeGroup0();
			break;
		case 1:
			// ECC
			if(groupVer == GROUP_VER_A) decodeGroup1A();
			if(groupVer == GROUP_VER_B) decodeGroupODA();
			break;
		case 2:
			// RT
			decodeGroup2();
			break;
		case 3:
			if(groupVer == GROUP_VER_A) decodeGroup3A();
			if(groupVer == GROUP_VER_B) decodeGroupODA();
			break;
		case 4:
			// CT
			if(groupVer == GROUP_VER_A) decodeGroup4A();
			if(groupVer == GROUP_VER_B) decodeGroupODA();
			break;
		case 10:
			// PTYN
			if(groupVer == GROUP_VER_A) decodeGroup10A();
			if(groupVer == GROUP_VER_B) decodeGroupODA();
			break;
		case 14:
			// TODO: handle EON
			break;
		case 15:
			// LPS
			if(groupVer == GROUP_VER_A) decodeGroup15A();
			if(groupVer == GROUP_VER_B) decodeGroup15B();
			break;
		default:
			decodeGroupODA();
			break;
		}
	}

	std::string Decoder::base26ToCall(uint16_t pi) {
		// Determin first better based on offset
		bool w = (pi >= 21672);
		std::string callsign(w ? "W" : "K");

		// Base25 decode the rest
		std::string restStr;
		int rest = pi - (w ? 21672 : 4096);
		while (rest) {
			restStr += 'A' + (rest % 26);
			rest /= 26;
		}

		// Pad with As
		while (restStr.size() < 3) {
			restStr += 'A';
		}

		// Reorder chars
		for (int i = restStr.size() - 1; i >= 0; i--) {
			callsign += restStr[i];
		}

		return callsign;
	}

	std::string Decoder::decodeCallsign(uint16_t pi) {
		if ((pi >> 8) == 0xAF) {
			// AFXY -> XY00
			return base26ToCall((pi & 0xFF) << 8);
		}
		else if ((pi >> 12) == 0xA) {
			// AXYZ -> X0YZ
			return base26ToCall((((pi >> 8) & 0xF) << 12) | (pi & 0xFF));
		}
		else if (pi >= 0x9950 && pi <= 0x9EFF) {
			// 3 letter callsigns
			if (THREE_LETTER_CALLS.find(pi) != THREE_LETTER_CALLS.end()) {
				return THREE_LETTER_CALLS[pi];
			}
			else {
				return "Not Assigned";
			}
		}
		else if (pi >= 0x1000 && pi <= 0x994F) {
			// Normal encoding
			if ((pi & 0xFF) == 0 || ((pi >> 8) & 0xF) == 0) {
				return "Not Assigned";
			}
			else {
				return base26ToCall(pi);
			}
		}
		else if (pi >= 0xB000 && pi <= 0xEFFF) {
			uint16_t _pi = ((pi >> 12) << 8) | (pi & 0xFF);
			if (NAT_LOC_LINKED_STATIONS.find(_pi) != NAT_LOC_LINKED_STATIONS.end()) {
				return NAT_LOC_LINKED_STATIONS[_pi];
			}
			else {
				return "Not Assigned";
			}
		}
		else {
			return "Not Assigned";
		}
	}

	void Decoder::reset() {
		piCode = 0;
		programCoverage = AREA_COVERAGE_LOCAL;
		callsign = "";

		groupType = 0;
		groupVer = GROUP_VER_A;
		trafficProgram = false;
		programType = PROGRAM_TYPE_EU_NONE;

		trafficAnnouncement = false;

		decoderIdent = 0;
		alternativeFrequency = 0;
		ps = "        ";

		radioText = "                                                                ";
		lastRTAB = false;

		lastPTYNAB = false;
		programTypeName = "        ";

		ecc = 0;

		longPS = "                                ";

		clock_hour = 0;
		clock_minute = 0;
		clock_offset_sense = false;
		clock_offset = 0;
		clock_mjd = 0;

		afCount = 0;
		afState = 0;
		afLfMfIncoming = 0;
		afs.fill(0);

		oda_aid_count = 0;
		odas_aid.fill(ODAAID{});

		ert = "                                                                                                                                ";
		ert_ucs2 = false;
        ert_direction = false;

		rtp_content_type_1 = 0;
        rtp_content_type_1_start = 0;
        rtp_content_type_1_len = 0;

        rtp_content_type_2 = 0;
        rtp_content_type_2_start = 0;
        rtp_content_type_2_len = 0;

		rtp_item_running = false;
        rtp_item_toggle = false;

		blockALastUpdate = std::chrono::high_resolution_clock::time_point();
		blockBLastUpdate = std::chrono::high_resolution_clock::time_point();
		group0LastUpdate = std::chrono::high_resolution_clock::time_point();
		group1LastUpdate = std::chrono::high_resolution_clock::time_point();
		group2LastUpdate = std::chrono::high_resolution_clock::time_point();
		group10ALastUpdate = std::chrono::high_resolution_clock::time_point();
		group15ALastUpdate = std::chrono::high_resolution_clock::time_point();
		eccLastUpdate = std::chrono::high_resolution_clock::time_point();
		rtpLastUpdate = std::chrono::high_resolution_clock::time_point();
	}

	bool Decoder::blockAValid() {
		auto now = std::chrono::high_resolution_clock::now();
		return (std::chrono::duration_cast<std::chrono::milliseconds>(now - blockALastUpdate)).count() < RDS_BLOCK_A_TIMEOUT_MS;
	}

	bool Decoder::blockBValid() {
		auto now = std::chrono::high_resolution_clock::now();
		return (std::chrono::duration_cast<std::chrono::milliseconds>(now - blockBLastUpdate)).count() < RDS_BLOCK_B_TIMEOUT_MS;
	}

	bool Decoder::group0Valid() {
		auto now = std::chrono::high_resolution_clock::now();
		return (std::chrono::duration_cast<std::chrono::milliseconds>(now - group0LastUpdate)).count() < RDS_GROUP_0_TIMEOUT_MS;
	}

	bool Decoder::eccValid() {
		auto now = std::chrono::high_resolution_clock::now();
		return (std::chrono::duration_cast<std::chrono::milliseconds>(now - eccLastUpdate)).count() < RDS_ECC_TIMEOUT_MS;
	}

	bool Decoder::group2Valid() {
		auto now = std::chrono::high_resolution_clock::now();
		return (std::chrono::duration_cast<std::chrono::milliseconds>(now - group2LastUpdate)).count() < RDS_GROUP_2_TIMEOUT_MS;
	}

	bool Decoder::group10AValid() {
		auto now = std::chrono::high_resolution_clock::now();
		return (std::chrono::duration_cast<std::chrono::milliseconds>(now - group10ALastUpdate)).count() < RDS_GROUP_10_TIMEOUT_MS;
	}

	bool Decoder::group15AValid() {
		auto now = std::chrono::high_resolution_clock::now();
		return (std::chrono::duration_cast<std::chrono::milliseconds>(now - group15ALastUpdate)).count() < RDS_GROUP_15_TIMEOUT_MS;
	}

	bool Decoder::groupRTPvalid() {
		auto now = std::chrono::high_resolution_clock::now();
		return (std::chrono::duration_cast<std::chrono::milliseconds>(now - rtpLastUpdate)).count() < RDS_GROUP_RTP_TIMEOUT_MS;
	}
	bool Decoder::groupERTvalid() {
		auto now = std::chrono::high_resolution_clock::now();
		return (std::chrono::duration_cast<std::chrono::milliseconds>(now - ertLastUpdate)).count() < RDS_GROUP_ERT_TIMEOUT_MS;
	}
}