#pragma once
#include <string>
#include <unordered_map>

namespace rds {
	inline std::string convert_from_rdscharset(const char* rds_str) {
		static const std::unordered_map<unsigned char, std::string> rds_to_utf8 = {
			// Control characters (optional: skip or replace)
			{0x00, " "}, {0x01, " "}, {0x02, " "}, {0x03, " "},
			{0x04, " "}, {0x05, " "}, {0x06, " "}, {0x07, " "},
			{0x08, " "}, {0x09, " "}, {0x0A, " "}, {0x0B, " "},
			{0x0C, " "}, {0x0D, " "}, {0x0E, " "}, {0x0F, " "},
			{0x10, " "}, {0x11, " "}, {0x12, " "}, {0x13, " "},
			{0x14, " "}, {0x15, " "}, {0x16, " "}, {0x17, " "},
			{0x18, " "}, {0x19, " "}, {0x1A, " "}, {0x1B, " "},
			{0x1C, " "}, {0x1D, " "}, {0x1E, " "}, {0x1F, " "},
			{0x7F, " "},

			// Extended chars
			{0x80, "\xE2\x82\xAC"}, // €
			{0x81, "'"},
			{0x82, ","},
			{0x83, "-"},
			{0x84, "."},
			{0x85, "/"},
			{0x86, ":"},
			{0x87, ";"},
			{0x88, "?"},
			{0x89, "!"},
			{0x8A, "\""},
			{0x8B, "("},
			{0x8C, ")"},
			{0x8D, "*"},
			{0x8E, "\xC2\xA1"}, // ¡
			{0x8F, "#"},
			{0x90, "&"},
			{0x91, "'"},
			{0x92, "\""},
			{0x93, "%"},
			{0x94, "+"},
			{0x95, "="},
			{0x96, "<"},
			{0x97, ">"},
			{0x98, "\xC2\xBF"}, // ¿
			{0x99, "["},
			{0x9A, "]"},
			{0x9B, "^"},
			{0x9C, "_"},
			{0x9D, "`"},
			{0x9E, "{"},
			{0x9F, "}"},
			{0xA0, "\xC2\xA0"}, // non-breaking space
			{0xA1, "\xC2\xA1"}, // ¡
			{0xA2, "\xC2\xA9"}, // ©
			{0xA3, "\xC2\xA3"}, // £
			{0xA4, "\xE2\x82\xA4"}, // ₤
			{0xA5, "\xC2\xA5"}, // ¥
			{0xA6, "|"},
			{0xA7, "\xC2\xA7"}, // §
			{0xA8, "\xC2\xA4"}, // ¤
			{0xA9, "\xC2\xAB"}, // «
			{0xAA, "$"},         // Dollar sign
			{0xAB, "\xC2\xBB"}, // »
			{0xAC, "\xC2\xAC"}, // ¬
			{0xAD, "-"},
			{0xAE, "\xC2\xAE"}, // ®
			{0xAF, "\xE2\x84\xA2"}, // ™
			{0xB0, "\xC2\xBA"}, // º
			{0xB1, "\xC2\xB9"}, // ¹
			{0xB2, "\xC2\xB2"}, // ²
			{0xB3, "\xC2\xB3"}, // ³
			{0xB4, "\xC2\xB1"}, // ±
			{0xB5, "\xC2\xB5"}, // µ
			{0xB6, "\xC2\xB6"}, // ¶
			{0xB7, "\xC2\xB7"}, // ·
			{0xB8, "\xC2\xB8"}, // ¸
			{0xB9, "\xC2\xB9"}, // ¹
			{0xBA, "\xC2\xBA"}, // º
			{0xBB, "\xC2\xB0"}, // °
			{0xBC, "\xC2\xBC"}, // ¼
			{0xBD, "\xC2\xBD"}, // ½
			{0xBE, "\xC2\xBE"}, // ¾
			{0xBF, "\xC2\xBF"}, // ¿
			// You can add more special symbols if needed
		};

		std::string utf8_str;
		unsigned char ch;

		while ((ch = *reinterpret_cast<const unsigned char*>(rds_str++))) {
			if (ch < 0x80) {
				utf8_str += ch; // Standard ASCII
			} else {
				auto it = rds_to_utf8.find(ch);
				if (it != rds_to_utf8.end()) {
					utf8_str += it->second;
				} else {
					utf8_str += '?'; // Unknown character
				}
			}
		}

		return utf8_str;
	}
}