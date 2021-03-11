#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <filesystem>

#include <arbornml/arbornml.hpp>
#include <arbornml/nmlexcept.hpp>
#include <arborio/swcio.hpp>
#include <arbor/morph/morphology.hpp>

#include "utils.hpp"

namespace io {

struct loaded_morphology {
    arb::morphology morph;
    std::vector<std::pair<std::string, std::string>> regions;
    std::vector<std::pair<std::string, std::string>> locsets;
};

loaded_morphology load_swc(const std::filesystem::path &fn, std::function<arb::morphology(const std::vector<arborio::swc_record> &)> swc_to_morph);
loaded_morphology load_allen_swc(const std::filesystem::path &fn);
loaded_morphology load_neuron_swc(const std::filesystem::path &fn);
loaded_morphology load_arbor_swc(const std::string &fn);
loaded_morphology load_neuroml(const std::filesystem::path &fn);

static std::unordered_map<std::string,
                          std::unordered_map<std::string,
                                             std::function<loaded_morphology(const std::filesystem::path &fn)>>>
loaders{{".swc", {{"Arbor",   [](const std::filesystem::path& fn) { return load_arbor_swc(fn); }},
                  {"Allen",   [](const std::filesystem::path& fn) { return load_allen_swc(fn); }},
                  {"Neuron",  [](const std::filesystem::path& fn) { return load_neuron_swc(fn); }}}},
        {".nml", {{"Default", [](const std::filesystem::path &fn) { return load_neuroml(fn); }}}}};

const std::vector<std::string>& get_suffixes();

const std::vector<std::string>& get_flavors(const std::string& suffix);

struct loader_state {
    std::string message;
    std::optional<std::function<loaded_morphology(const std::filesystem::path&)>> load;
};

loader_state get_loader(const std::string &extension, const std::string &flavor);

}
