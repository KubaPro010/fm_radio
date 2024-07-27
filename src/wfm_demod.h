#pragma once
#include "demod.h"
#include <dsp/demod/broadcast_fm.h>
#include "rds_demod.h"
#include <gui/widgets/symbol_diagram.h>
#include <fstream>
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
                    ImGui::Text("%s %s %s %s",
                        ((rdsDecode.getDi() & 1) == 1) ? "Stereo":"Mono",
                        ((rdsDecode.getDi() & 2) == 2) ? "Artificial Head":"",
                        ((rdsDecode.getDi() & 4) == 4) ? "Compressed":"",
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

                if (rdsDecode.musicValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("M/S");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", rdsDecode.getMusic() ? "Music":"Speech");
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("M/S");
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

                if (rdsDecode.licValid()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("LIC");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%03X", rdsDecode.getLic());
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("LIC");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted("---");
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

            char buf[154];
            if (_this->rdsDecode.PSNameValid() && _this->rdsDecode.radioTextValid() && _this->rdsDecode.LPSNameValid()) {
                sprintf(buf, "Radio Data System Information:\n\tPS: %s\n\tLPS: %s\n\tRT (%s): %s", _this->rdsDecode.getPSName().c_str(), _this->rdsDecode.getLPSName().c_str(), _this->rdsDecode.getRadioTextAB().c_str(), _this->rdsDecode.getRadioText().c_str());
            } else if (_this->rdsDecode.PSNameValid() && _this->rdsDecode.radioTextValid()) {
                sprintf(buf, "Radio Data System Information:\n\tPS: %s\n\tLPS: -\n\tRT (%s): %s", _this->rdsDecode.getPSName().c_str(), _this->rdsDecode.getRadioTextAB().c_str(), _this->rdsDecode.getRadioText().c_str());
            }
            else if (_this->rdsDecode.LPSNameValid() && _this->rdsDecode.PSNameValid()) {
                sprintf(buf, "Radio Data System Information:\n\tPS: %s\n\tLPS: %s\n\tRT (-): -", _this->rdsDecode.getPSName().c_str(), _this->rdsDecode.getLPSName().c_str());
            }
            else if (_this->rdsDecode.radioTextValid() && _this->rdsDecode.LPSNameValid()) {
                sprintf(buf, "Radio Data System Information:\n\tPS: -\n\tLPS: %s\n\tRT (%s): %s", _this->rdsDecode.getLPSName().c_str(), _this->rdsDecode.getRadioTextAB().c_str(), _this->rdsDecode.getRadioText().c_str());
            }
            else if (_this->rdsDecode.LPSNameValid()) {
                sprintf(buf, "Radio Data System Information:\n\tPS: -\n\tLPS: %s\n\tRT (-): -", _this->rdsDecode.getLPSName().c_str());
            }
            else if (_this->rdsDecode.PSNameValid()) {
                sprintf(buf, "Radio Data System Information:\n\tPS: %s\n\tLPS: -\n\tRT (-): -", _this->rdsDecode.getPSName().c_str());
            }
            else if (_this->rdsDecode.radioTextValid()) {
                sprintf(buf, "Radio Data System Information:\n\tPS: -\n\tLPS: -\n\tRT (%s): %s", _this->rdsDecode.getRadioTextAB().c_str(), _this->rdsDecode.getRadioText().c_str());
            }
            else {
                return;
            }

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