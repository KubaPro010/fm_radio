#pragma once
#include "demod.h"
#include <dsp/demod/broadcast_fm.h>
#include "rds_demod.h"
#include <gui/widgets/symbol_diagram.h>
#include <fstream>
#include <iomanip>
#include <rds.h>

namespace demod {
    enum RDSRegion {
        RDS_REGION_EUROPE,
        RDS_REGION_NORTH_AMERICA
    };

    class WFM : public Demodulator {
    public:
        WFM() : diag(0.5, 4096)  {}

        WFM(std::string name, ConfigManager* config, dsp::stream<dsp::complex_t>* input, double bandwidth, double audioSR) : diag(0.5, 4096) {
            init(name, config, input, bandwidth, audioSR);
        }

        ~WFM() {
            stop();
            gui::waterfall.onFFTRedraw.unbindHandler(&fftRedrawHandler);
        }

        void init(std::string name, ConfigManager* config, dsp::stream<dsp::complex_t>* input, double bandwidth, double audioSR) {
            this->name = name;
            _config = config;

            // Define RDS regions
            rdsRegions.define("eu", "Europe", RDS_REGION_EUROPE);
            rdsRegions.define("na", "North America", RDS_REGION_NORTH_AMERICA);

            // Register FFT draw handler
            fftRedrawHandler.handler = fftRedraw;
            fftRedrawHandler.ctx = this;
            gui::waterfall.onFFTRedraw.bindHandler(&fftRedrawHandler);

            // Default
            std::string rdsRegionStr = "eu";

            // Load config
            _config->acquire();
            bool modified = false;
            if (config->conf[name].contains("stereo")) {
                _stereo = config->conf[name]["stereo"];
            }
            if (config->conf[name].contains("lowPass")) {
                _lowPass = config->conf[name]["lowPass"];
            }
            if (config->conf[name].contains("rds")) {
                _rds = config->conf[name]["rds"];
            }
            if (config->conf[name].contains("rdsInfo")) {
                _rdsInfo = config->conf[name]["rdsInfo"];
            }
            if (config->conf[name].contains("rdsRegion")) {
                rdsRegionStr = config->conf[name]["rdsRegion"];
            }
            _config->release(modified);

            // Load RDS region
            if (rdsRegions.keyExists(rdsRegionStr)) {
                rdsRegionId = rdsRegions.keyId(rdsRegionStr);
                rdsRegion = rdsRegions.value(rdsRegionId);
            }
            else {
                rdsRegion = RDS_REGION_EUROPE;
                rdsRegionId = rdsRegions.valueId(rdsRegion);
            }

            // Init DSP
            demod.init(input, bandwidth / 2.0f, getIFSampleRate(), _stereo, _lowPass, _rds);
            rdsDemod.init(&demod.rdsOut, _rdsInfo);
            hs.init(&rdsDemod.out, rdsHandler, this);
            reshape.init(&rdsDemod.soft, 4096, (1187 / 30) - 4096);
            diagHandler.init(&reshape.out, _diagHandler, this);

            // Init RDS display
            diag.lines.push_back(-0.8);
            diag.lines.push_back(0.8);
        }

        void start() {
            demod.start();
            rdsDemod.start();
            hs.start();
            reshape.start();
            diagHandler.start();
        }

        void stop() {
            demod.stop();
            rdsDemod.stop();
            hs.stop();
            reshape.stop();
            diagHandler.stop();
        }

        void showMenu() {
            if (ImGui::Checkbox(("Stereo##_radio_wfm_stereo_" + name).c_str(), &_stereo)) {
                setStereo(_stereo);
                _config->acquire();
                _config->conf[name]["stereo"] = _stereo;
                _config->release(true);
            }
            if (ImGui::Checkbox(("Low Pass##_radio_wfm_lowpass_" + name).c_str(), &_lowPass)) {
                demod.setLowPass(_lowPass);
                _config->acquire();
                _config->conf[name]["lowPass"] = _lowPass;
                _config->release(true);
            }
            if (ImGui::Checkbox(("Decode RDS##_radio_wfm_rds_" + name).c_str(), &_rds)) {
                demod.setRDSOut(_rds);
                _config->acquire();
                _config->conf[name]["rds"] = _rds;
                _config->release(true);
            }

            if (!_rds) { ImGui::BeginDisabled(); }
            if (ImGui::Checkbox(("Advanced RDS Info##_radio_wfm_rds_info_" + name).c_str(), &_rdsInfo)) {
                setAdvancedRds(_rdsInfo);
                _config->acquire();
                _config->conf[name]["rdsInfo"] = _rdsInfo;
                _config->release(true);
            }
            ImGui::SameLine();
            ImGui::FillWidth();
            if (ImGui::Combo(("##_radio_wfm_rds_region_" + name).c_str(), &rdsRegionId, rdsRegions.txt)) {
                rdsRegion = rdsRegions.value(rdsRegionId);
                _config->acquire();
                _config->conf[name]["rdsRegion"] = rdsRegions.key(rdsRegionId);
                _config->release(true);
            }
            if (!_rds) { ImGui::EndDisabled(); }

            float menuWidth = ImGui::GetContentRegionAvail().x;

            if (_rds && _rdsInfo) {
                ImGui::BeginTable(("##radio_wfm_rds_info_tbl_" + name).c_str(), 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders);
                if (rdsDecode.piCodeValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("PI Code");
                    ImGui::TableSetColumnIndex(1);
                    if (rdsRegion == RDS_REGION_NORTH_AMERICA) {
                        ImGui::Text("%04X (%s)", rdsDecode.getPICode(), rdsDecode.getCallsign().c_str());
                    }
                    else {
                        ImGui::Text("%04X", rdsDecode.getPICode());
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Program Coverage");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s (%d)", rds::AREA_COVERAGE_TO_STR[rdsDecode.getProgramCoverage()], rdsDecode.getProgramCoverage());
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("PI Code");
                    ImGui::TableSetColumnIndex(1);
                    if (rdsRegion == RDS_REGION_NORTH_AMERICA) {
                        ImGui::TextUnformatted("---- (----)");
                    }
                    else {
                        ImGui::TextUnformatted("----");
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Program Coverage");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted("------- (--)");
                }

                if (rdsDecode.programTypeValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Program Type");
                    ImGui::TableSetColumnIndex(1);
                    if (rdsRegion == RDS_REGION_NORTH_AMERICA) {
                        ImGui::Text("%s (%d)", rds::PROGRAM_TYPE_US_TO_STR[rdsDecode.getProgramType()], rdsDecode.getProgramType());
                    }
                    else {
                        ImGui::Text("%s (%d)", rds::PROGRAM_TYPE_EU_TO_STR[rdsDecode.getProgramType()], rdsDecode.getProgramType());
                    }
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Program Type");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted("------- (--)");
                }

                if (rdsDecode.afValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("AF");
                    ImGui::TableSetColumnIndex(1);

                    std::array<uint32_t, 25> arr = rdsDecode.getAFs();
                    uint8_t count = rdsDecode.getAFCount();

                    if (count > arr.size()) {
                        count = arr.size();
                    }

                    std::stringstream ss;
                    for (int j = 0; j < count; ++j) {
                        if (arr[j] == 0) {
                            continue;
                        }
                        if (j > 0) {
                            ss << " ";
                        }
                        ss << static_cast<int>(arr[j]);
                    }
                    ImGui::Text("%s", ss.str().c_str());
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("AF");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted("---");
                }

                if (rdsDecode.odaAIDValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("ODA AID");
                    ImGui::TableSetColumnIndex(1);

                    std::array<rds::ODAAID, 8> arr = rdsDecode.getOdaAID();
                    uint8_t count = rdsDecode.getOdaAIDCount();

                    if (count > arr.size()) {
                        count = arr.size();
                    }

                    std::stringstream ss;
                    for (int j = 0; j < count; ++j) {
                        if (arr[j].AID == 0) {
                            continue;
                        }
                        if (j > 0) {
                            ss << " ";
                        }
                        ss << static_cast<int>(arr[j].GroupType);
                        if(arr[j].GroupVer == rds::GROUP_VER_A) ss << "A";
                        else ss << "B";
                        ss << ":" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << arr[j].AID;
                    }

                    ImGui::Text("%s", ss.str().c_str());
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("ODA AID");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted("---");
                }

                if (rdsDecode.programTypeNameValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("PTYN");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", rdsDecode.getProgramTypeName().c_str());
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("PTYN");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted("---");
                }

                if (rdsDecode.diValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Decoder ID");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s",
                        ((rdsDecode.getDi() & 8) == 8) ? "Dynamic PTY":"Static PTY");
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Decoder ID");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted("---");
                }

                if (rdsDecode.tpValid() && rdsDecode.taValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("TP TA");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s %s", rdsDecode.getTp() ? "TP":"", rdsDecode.getTa() ? "TA":"");
                } else if (rdsDecode.tpValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("TP TA");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", rdsDecode.getTp() ? "TP":"");
                } else if (rdsDecode.taValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("TP TA");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", rdsDecode.getTa() ? "TA":"");
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("TP TA");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted("---");
                }

                if (rdsDecode.eccValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("ECC");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%02X", rdsDecode.getEcc());
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("ECC");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted("--");
                }

                if(rdsDecode.CTReceived()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Time");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%02d:%02d (%02d:%02d)",
                        (rdsDecode.getClockHour() + ((rdsDecode.getClockOffsetSense() ? -1 : 1) * (rdsDecode.getClockOffset() / 2))) % 24,
                        (rdsDecode.getClockMinute() + ((rdsDecode.getClockOffsetSense() ? -1 : 1) * (rdsDecode.getClockOffset() % 2) * 30)) % 60,
                        rdsDecode.getClockHour() % 24,
                        rdsDecode.getClockMinute() % 60
                    );

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Date");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%02d.%02d.%04d (DD.MM.YYYY)",
                        rdsDecode.getMJDDay(rdsDecode.getClockMJD()),
                        rdsDecode.getMJDMonth(rdsDecode.getClockMJD()),
                        rdsDecode.getMJDYear(rdsDecode.getClockMJD())
                    );
                } else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Time");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("--:-- (--:--)");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Date");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("--.--.---- (DD.MM.YYYY)");
                }


                ImGui::EndTable();

                if(ImGui::Button("Reset", ImVec2(menuWidth, 0))) {
                    rdsDecode.reset();
                }

                ImGui::SetNextItemWidth(menuWidth);
                diag.draw();
            }
        }

        void setBandwidth(double bandwidth) {
            demod.setDeviation(bandwidth / 2.0f);
        }

        void setInput(dsp::stream<dsp::complex_t>* input) {
            demod.setInput(input);
        }

        void FrequencyChanged() {
            // TODO: VFO doesnt tell the frequency selected, hereby we have no idea what frequency is selected so we cant tell if it changed, thanks Ryzerth 🤦
            rdsDecode.reset();
        }

        // ============= INFO =============

        double getIFSampleRate() { return 250000.0; }
        double getAFSampleRate() { return getIFSampleRate(); }
        double getDefaultBandwidth() { return 150000.0; }
        double getMinBandwidth() { return 50000.0; }
        double getMaxBandwidth() { return getIFSampleRate(); }
        double getDefaultSnapInterval() { return 100000.0; }
        int getVFOReference() { return ImGui::WaterfallVFO::REF_CENTER; }
        int getDefaultDeemphasisMode() { return DEEMP_MODE_50US; }
        dsp::stream<dsp::stereo_t>* getOutput() { return &demod.out; }

        // ============= DEDICATED FUNCTIONS =============

        void setStereo(bool stereo) {
            _stereo = stereo;
            demod.setStereo(_stereo);
        }

        void setAdvancedRds(bool enabled) {
            rdsDemod.setSoftEnabled(enabled);
            _rdsInfo = enabled;
        }

    private:
        static void rdsHandler(uint8_t* data, int count, void* ctx) {
            WFM* _this = (WFM*)ctx;
            _this->rdsDecode.process(data, count);
        }

        static void _diagHandler(float* data, int count, void* ctx) {
            WFM* _this = (WFM*)ctx;
            float* buf = _this->diag.acquireBuffer();
            memcpy(buf, data, count * sizeof(float));
            _this->diag.releaseBuffer();
        }

        static void fftRedraw(ImGui::WaterFall::FFTRedrawArgs args, void* ctx) {
            WFM* _this = (WFM*)ctx;
            if (!_this->_rds) { return; }

            std::string ps = _this->rdsDecode.PSNameValid() ? _this->rdsDecode.getPSName() : "-";
            std::string lps = _this->rdsDecode.LPSNameValid() ? _this->rdsDecode.getLPSName() : "-";
            std::string rt = _this->rdsDecode.radioTextValid() ? _this->rdsDecode.getRadioText() : "-";
            std::string rtAB = _this->rdsDecode.radioTextValid() ? _this->rdsDecode.getRadioTextAB() : "-";
            std::string ert = _this->rdsDecode.ertValid() ? _this->rdsDecode.getERT() : "-";

            bool rtp_running = _this->rdsDecode.getRTPRunning();
            bool rtp_toggle = _this->rdsDecode.getRTPToggle();
            std::string rtp1_type = rtp_running ? rds::RTP_TO_STR[_this->rdsDecode.getRTPContentType1()] : "-";
            std::string rtp1 = rtp_running ? rt.substr(_this->rdsDecode.getRTPContentType1Start(), _this->rdsDecode.getRTPContentType1Len()) : "-";
            std::string rtp2_type = rtp_running ? rds::RTP_TO_STR[_this->rdsDecode.getRTPContentType2()] : "-";
            std::string rtp2 = rtp_running ? rt.substr(_this->rdsDecode.getRTPContentType2Start(), _this->rdsDecode.getRTPContentType2Len()) : "-";

            std::ostringstream oss;
            oss << "Radio Data System Information:\n"
                << "\tPS: " << ps << "\n"
                << "\tLPS: " << lps << "\n"
                << "\tRT (" << rtAB << "): " << rt << "\n"
                << "\t\tRT+ Toggle: " << (rtp_toggle ? "B" : "A") << "\n"
                << "\t\tRT+ 1 - " << rtp1_type << ": " << rtp1 << "\n"
                << "\t\tRT+ 2 - " << rtp2_type << ": " << rtp2 << "\n"
                << "\tERT: " << ert;

            std::string output = oss.str();
            const char* buf = output.c_str();

            // Calculate paddings
            ImVec2 min = args.min;
            min.x += 5.0f * style::uiScale;
            min.y += 5.0f * style::uiScale;
            ImVec2 tmin = min;
            tmin.x += 5.0f * style::uiScale;
            tmin.y += 5.0f * style::uiScale;
            ImVec2 tmax = ImGui::CalcTextSize(buf);
            tmax.x += tmin.x;
            tmax.y += tmin.y;
            ImVec2 max = tmax;
            max.x += 5.0f * style::uiScale;
            max.y += 5.0f * style::uiScale;

            // Draw back drop
            args.window->DrawList->AddRectFilled(min, max, IM_COL32(0, 0, 0, 255), 0.4f);

            // Draw text
            args.window->DrawList->AddText(NULL, args.window->DrawList->_Data->FontSize * 1.15, tmin, IM_COL32(255, 255, 255, 255), buf);
        }

        dsp::demod::BroadcastFM demod;
        RDSDemod rdsDemod;
        dsp::sink::Handler<uint8_t> hs;
        EventHandler<ImGui::WaterFall::FFTRedrawArgs> fftRedrawHandler;

        dsp::buffer::Reshaper<float> reshape;
        dsp::sink::Handler<float> diagHandler;
        ImGui::SymbolDiagram diag;

        rds::Decoder rdsDecode;

        ConfigManager* _config = NULL;

        bool _stereo = false;
        bool _lowPass = true;
        bool _rds = false;
        bool _rdsInfo = false;

        int rdsRegionId = 0;
        RDSRegion rdsRegion = RDS_REGION_EUROPE;
        OptionList<std::string, RDSRegion> rdsRegions;

        std::string name;
    };
}
