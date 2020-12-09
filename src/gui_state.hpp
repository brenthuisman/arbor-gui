#pragma once

#include <unordered_set>
#include <string>
#include <vector>
#include <filesystem>
#include <deque>

#include <arbor/cable_cell.hpp>
#include <arbor/morph/place_pwlin.hpp>
#include <arbor/morph/mprovider.hpp>
#include <arbor/morph/primitives.hpp>
#include <arbor/morph/morphexcept.hpp>
#include <arbor/morph/label_parse.hpp>
#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>
#include <arbor/mechcat.hpp>
#include <arbor/mechinfo.hpp>
#include <arborio/swcio.hpp>

#include <definition.hpp>
#include <location.hpp>
#include <geometry.hpp>
#include <cell_builder.hpp>

// definitions for placings
struct prb_def: definition {
    std::string locset_name = "";
    double frequency = 1000; // [Hz]
    std::string variable = "voltage";
};

struct stm_def: definition {
    std::string locset_name = "";
    double delay = 0;      // [ms]
    double duration = 0;   // [ms]
    double amplitude = 0;  // [nA]
};

struct sdt_def: definition {
    std::string locset_name = "";
    double threshold = 0;  // [mV]
};

// definitions for paintings
struct ion_def: definition {
    std::optional<double> Xi;
    std::optional<double> Xo;
    std::optional<double> Er;
};

struct par_def: definition {
    std::optional<double> TK, Cm, Vm, RL;
};

struct mech_def: definition {
    std::string name = "";
    std::unordered_map<std::string, double> parameters  = {};
    std::unordered_map<std::string, double> global_vars = {};
};

struct ion_default {
    std::string name = "";
    double Xi = 0;
    double Xo = 0;
    double Er = 0;
    std::string method = "constant";
};

struct par_default {
    double TK, Cm, Vm, RL;
};

struct file_chooser_state {
    std::filesystem::path cwd = std::filesystem::current_path();
    std::optional<std::string> filter = {};
    bool show_hidden;
    bool use_filter;
    std::filesystem::path file;
};

struct gui_state {
    // Interface to Arbor morphology
    cell_builder builder;

    // rendering
    geometry renderer;
    std::vector<renderable> render_regions = {};
    std::vector<renderable> render_locsets = {};

    // placeables
    std::vector<ls_def>  locset_defs   = {};
    std::vector<prb_def> probe_defs    = {};
    std::vector<sdt_def> detector_defs = {};
    std::vector<stm_def> iclamp_defs   = {};

    // paintings
    std::vector<reg_def> region_defs                  = {};
    par_default parameter_defaults                    = {};
    std::vector<ion_default> ion_defaults             = {};
    std::vector<par_def> parameter_defs               = {};
    std::vector<std::vector<ion_def>> ion_defs        = {};
    std::vector<std::vector<mech_def>> mechanism_defs = {};

    file_chooser_state file_chooser;

    gui_state(const gui_state&) = delete;
    gui_state();

    void load_allen_swc(const std::string& fn);
    void load_neuron_swc(const std::string& fn);
    void load_arbor_swc(const std::string& fn);
    void load_neuroml(const std::string& fn);

    void serialize(const std::string& fn);
    void deserialize(const std::string& fn);

    void update();
    void reset();
    void gui();
};
