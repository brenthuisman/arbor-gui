namespace arb {
    void to_json(json& j, const cable_cell_ion_data& d) {
        j = {{"int_concentration",  d.init_int_concentration.value_or(NAN)},
             {"ext_concentration",  d.init_ext_concentration.value_or(NAN)},
             {"reversal_potential", d.init_reversal_potential.value_or(NAN)}};
    }

    void to_json(json& j, const mechanism_desc& d) {
        json ps;
        for (const auto& [k, v]: d.values()) {
            ps[k] = v;
        }
        j = json{{"name", d.name()},
                 {"parameters", ps}};
    }

    std::string to_string(const arb::locset& ls) {
        std::stringstream ss;
        ss << ls;
        return ss.str();
    }

    std::string to_string(const arb::region& ls) {
        std::stringstream ss;
        ss << ls;
        return ss.str();
    }
}

struct morphology {
    arb::segment_tree tree;
    arb::label_dict   label;
    arb::morphology   morph;
    arb::place_pwlin  pwlin;

    morphology(): tree{}, label{}, morph{}, pwlin{morph} {};
    morphology(const arb::segment_tree& t): tree{t}, label{}, morph{t}, pwlin{morph} {}

    std::optional<renderable> make_renderable(geometry& renderer, reg_def& def) {
        if (!def.data) return {};
        log_info("Making frustrums for region {} '{}'", def.name, def.definition);
        auto provider = arb::mprovider{morph, label};
        auto region   = def.data.value();
        auto concrete = thingify(region, provider);
        auto segments = pwlin.all_segments(concrete);
        return {renderer.make_region(segments, {0.0f, 0.0f, 0.0f, 1.0f})};
    }

    std::optional<renderable> make_renderable(geometry& renderer, ls_def& def) {
        if (!def.data) return {};
        log_info("Making markers for locset {} '{}'", def.name, def.definition);
        auto locset   = def.data.value();
        auto provider = arb::mprovider{morph, label};
        auto concrete = thingify(locset, provider);
        std::vector<glm::vec3> points;
        for (const auto& loc: concrete) {
            for (const auto p: pwlin.all_at(loc)) {
                points.emplace_back((float) p.x, (float) p.y, (float) p.z);
            }
        }
        return {renderer.make_marker(points, {0.0f, 0.0f, 0.0f, 1.0f})};
    }

    auto make_cell() { return arb::cable_cell(morph, label); }
};

struct region {
    std::unordered_map<std::string, arb::mechanism_desc> mechanisms;
    arb::cable_cell_parameter_set values;
    arb::region location;
};

void to_json(json& j, const region& r) {
    json parameters = {{"membrane_capacitance",    r.values.membrane_capacitance.value_or(NAN)},
                       {"axial_resistivity",       r.values.axial_resistivity.value_or(NAN)},
                       {"temperature_K",           r.values.temperature_K.value_or(NAN)}};
    json mechs;
    for (const auto& [mech, desc]: r.mechanisms) {
        json d = desc;
        d["location"] = to_string(r.location);
        mechs.push_back(d);
    }
    json ions;
    for (const auto& [ion, desc]: r.values.ion_data) {
        json d = desc;
        d["name"] = ion;
        ions.push_back(d);
    }
    j = json{{"parameters", parameters},
             {"ions", ions},
             {"mechanisms", mechs}};
}

struct probe {
    arb::locset location;
    double frequency;
    std::string variable;
    probe(const arb::locset& s, double f, const std::string& v) : location{s}, frequency{f}, variable{v} {}
};

void to_json(json& j, const probe& p) {
    j = json{{"location", to_string(p.location)},
             {"frequency", p.frequency},
             {"variable", p.variable}};
}

struct stimulus {
    arb::locset location;
    arb::i_clamp clamp;
    stimulus(const arb::locset& s, double d, double l, double a) : location{s}, clamp{d, l, a} {}
};


void to_json(json& j, const stimulus& c) {
    j = json{{"location",  to_string(c.location)},
             {"delay",     c.clamp.delay},
             {"duration",  c.clamp.duration},
             {"amplitude", c.clamp.amplitude}};
}

struct simulation {
    double t_stop  = 1400;
    double dt = 1.0/200.0;
    std::vector<stimulus> stimuli = {{"\"center\"", 200, 1000, 0.15}};
    std::vector<probe> probes = {{"\"center\"", 1000.0, "voltage"}};
};

void to_json(json& j, const simulation& s) {
    j = json{{"t_stop", s.t_stop},
             {"dt", s.dt},
             {"probes", s.probes},
             {"stimuli", s.stimuli}};
}

struct parameters {
    arb::cable_cell_parameter_set values;
    std::unordered_map<std::string, region> regions;

    geometry renderer;

    std::vector<renderable> render_regions = {};
    std::vector<renderable> render_locsets = {};

    std::vector<reg_def> region_defs = {};
    std::vector<ls_def>  locset_defs = {};

    morphology morph;
    simulation sim;

    std::filesystem::path cwd = std::filesystem::current_path();

    parameters(const parameters&) = delete;


    parameters():
        values{arb::neuron_parameter_defaults}, morph{} {}

    void load_allen_swc(const std::string& swc_fn) {
        log_debug("Reading {}", swc_fn);
        render_regions.clear();
        render_locsets.clear();
        locset_defs.clear();
        region_defs.clear();
        std::ifstream in(swc_fn.c_str());
        auto tree = arb::as_segment_tree(arb::parse_swc(in, arb::swc_mode::relaxed));
        morph = morphology(std::move(tree));
        renderer = geometry{};
        renderer.load_geometry(morph.tree);
        add_swc_tags();
    }

    void add_swc_tags() {
        add_region("soma", "(tag 1)");
        add_region("axon", "(tag 2)");
        add_region("dend", "(tag 3)");
        add_region("apic", "(tag 4)");
        add_locset("center", "(location 0 0)");
    }

    void add_region(std::string_view l, std::string_view d) {
        region_defs.emplace_back(l, d);
        render_regions.emplace_back();
        render_regions.back().color = next_color();
    }

    void add_region() {
        region_defs.emplace_back();
        render_regions.emplace_back();
        render_regions.back().color = next_color();
    }

    void add_locset(std::string_view l, std::string_view d) {
        locset_defs.emplace_back(l, d);
        render_locsets.emplace_back();
        render_locsets.back().color = next_color();
    }

    void add_locset() {
        locset_defs.emplace_back();
        render_locsets.emplace_back();
        render_locsets.back().color = next_color();
    }

    auto render_cell(float width, float height) {
        if (region_defs.size() != render_regions.size()) log_fatal("Invariant!");
        for (auto ix = 0ul; ix < region_defs.size(); ++ix) {
            auto& def = region_defs[ix];
            auto& render = render_regions[ix];
            if (def.state == def_state::changed) {
                render.active = false;
                try {
                    auto maybe_render = morph.make_renderable(renderer, def);
                    if (maybe_render) {
                        auto tmp = maybe_render.value();
                        tmp.color = render.color;
                        render = tmp;
                    }
                    def.state = def_state::clean;
                } catch (arb::morphology_error& e) {
                    def.error(e.what());
                    continue;
                }
            }
        }
        if (locset_defs.size() != render_locsets.size()) log_fatal("Invariant!");
        for (auto ix = 0ul; ix < locset_defs.size(); ++ix) {
            auto& def = locset_defs[ix];
            auto& render = render_locsets[ix];
            if (def.state == def_state::changed) {
                render.active = false;
                try {
                    auto maybe_render = morph.make_renderable(renderer, def);
                    if (maybe_render) {
                        auto tmp = maybe_render.value();
                        tmp.color = render.color;
                        render = tmp;
                    }
                    def.state = def_state::clean;
                } catch (arb::morphology_error& e) {
                    def.error(e.what());
                    continue;
                }
            }
        }
        return renderer.render(zoom, phi, width, height, render_regions, render_locsets);
    }

    auto load_allen_fit(const std::string& dyn) {
        json genome;
        {
            std::ifstream fd(dyn.c_str());
            fd >> genome;
        }
        auto cond = genome["conditions"][0];
        values.axial_resistivity       = double{genome["passive"][0]["ra"]};
        values.temperature_K           = double{cond["celsius"]} + 273.15;
        values.init_membrane_potential = double{cond["v_init"]};

        for (auto& block: cond["erev"]) {
            const std::string& region = block["section"];
            for (auto& kv: block.items()) {
                if (kv.key() == "section") continue;
                std::string ion = kv.key();
                ion.erase(0, 1);
                regions[region].values.ion_data[ion].init_reversal_potential = double{kv.value()};
            }
        }

        for (auto& block: genome["genome"]) {
            std::string region = std::string{block["section"]};
            std::string name   = std::string{block["name"]};
            double value       = block["value"].is_string() ? std::stod(std::string{block["value"]}) : double{block["value"]};
            std::string mech   = std::string{block["mechanism"]};

            if (mech == "") {
                if (ends_with(name, "_pas")) {
                    mech = "pas";
                } else {
                    if (name == "cm") {
                        regions[region].values.membrane_capacitance = value/100.0;
                        continue;
                    }
                    if ((name == "ra") || (name == "Ra")) {
                        regions[region].values.axial_resistivity = value;
                        continue;
                    }
                    if (name == "vm") {
                        regions[region].values.init_membrane_potential = value;
                        continue;
                    }
                    if (name == "celsius") {
                        regions[region].values.temperature_K = value;
                        continue;
                    }
                    log_error("Unknown parameter {}", name);
                }
            }
            name.erase(name.size() - mech.size() - 1, mech.size() + 1);
            set_mech(region, mech, name, value);
        }
    }

    void set_mech(const std::string& region, const std::string& mech, const std::string& key, double value) {
        if (regions.find(region) == regions.end()) log_error("Unknown region");
        auto& mechs = regions[region].mechanisms;
        if (mechs.find(mech) == mechs.end()) mechs[mech] = arb::mechanism_desc{mech};
        mechs[mech].set(key, value);
    }

    void set_reversal_potential_method(const std::string& ion, int method) {
        if (values.ion_data.find(ion) == values.ion_data.end()) log_error("{}: Unknown ion: '{}'", __FUNCTION__, ion);
        auto& methods = values.reversal_potential_method;
        std::string nernst = fmt::format("nernst/x={}", ion);
        if      (method == 0) methods.erase(ion);
        else if (method == 1) methods[ion] = arb::mechanism_desc{nernst};
        else log_error("Unknown method: '{}'", method);
    }

    int get_reversal_potential_method(const std::string& ion) {
        if (values.ion_data.find(ion) == values.ion_data.end()) log_error("{}: Unknown ion: '{}'", __FUNCTION__, ion);
        auto& methods = values.reversal_potential_method;
        std::string nernst = fmt::format("nernst/x={}", ion);
        if (methods.find(ion) == methods.end()) return 0;
        if (methods[ion].name() == nernst)      return 1;
        log_fatal("Unknown method: '{}'", methods[ion].name());
    }

    std::string get_reversal_potential_method_as_string(const std::string& ion) {
        return reversal_potential_methods[get_reversal_potential_method(ion)];
    }

    static const char* reversal_potential_methods[];
};

const char* parameters::reversal_potential_methods[] = {"const", "nernst"};

void to_json(json& j, const parameters& p) {
    json parameters = {{"membrane_capacitance",    p.values.membrane_capacitance.value_or(NAN)},
                       {"axial_resistivity",       p.values.axial_resistivity.value_or(NAN)},
                       {"temperature_K",           p.values.temperature_K.value_or(NAN)},
                       {"init_membrane_potential", p.values.init_membrane_potential.value_or(NAN)}};
    json ions;
    for (const auto& [ion, desc]: p.values.ion_data) {
        ions[ion] = desc;
        if (p.values.reversal_potential_method.find(ion) != p.values.reversal_potential_method.end()) {
            ions[ion]["reversal_potential_method"] = p.values.reversal_potential_method.at(ion).name();
        } else {
            ions[ion]["reversal_potential_method"] = "const";
        }
    }
    json globals = json{{"ions", ions},
                        {"parameters", parameters}};
    json locals;
    json mechanisms;
    for (const auto& [region, data]: p.regions) {
        // Parameter settings
        json parameters = {{"membrane_capacitance",    data.values.membrane_capacitance.value_or(NAN)},
                           {"axial_resistivity",       data.values.axial_resistivity.value_or(NAN)},
                           {"temperature_K",           data.values.temperature_K.value_or(NAN)}};
        json ions;
        for (const auto& [ion, desc]: data.values.ion_data) {
            json d = desc;
            d["name"] = ion;
            ions.push_back(d);
        }
        json local = {{"location", to_string(data.location)},
                      {"parameters", parameters},
                      {"ions", ions}};
        locals.push_back(local);

        // mechanisms
        for (const auto& [mech, desc]: data.mechanisms) {
            json d = desc;
            d["location"] = to_string(data.location);
            mechanisms.push_back(d);
        }
    }

    j = json{{"electrical", {{"global", globals},
                             {"local",  locals},
                             {"mechanisms", mechanisms}}},
             {"simulation", p.sim},
             {"morphology",  {{"kind", "swc"},
                              {"file", "data/full.swc"}}}};
}

auto make_simulation(parameters& p) {
    log_info("Deriving simulation");
    auto cell   = p.morph.make_cell();

    // This takes care of
    //  * Baseline parameters: cm, Vm, Ra, T
    //  * Ion concentrations
    //  * Reversal potentials
    cell.default_parameters = p.values;

    // Now paint in the region-specifc values and place the clamps
    for (const auto& [region, region_data]: p.regions) {
        auto cm = region_data.values.membrane_capacitance.value_or(p.values.membrane_capacitance.value());
        cell.paint(region, arb::membrane_capacitance{cm});
        auto vm = region_data.values.init_membrane_potential.value_or(p.values.init_membrane_potential.value());
        cell.paint(region, arb::init_membrane_potential{vm});
        auto ra = region_data.values.axial_resistivity.value_or(p.values.axial_resistivity.value());
        cell.paint(region, arb::axial_resistivity{ra});
        auto T  = region_data.values.temperature_K.value_or(p.values.temperature_K.value());
        cell.paint(region, arb::temperature_K{T});
        for (const auto& [ion, data]: region_data.values.ion_data) {
            if (data.init_int_concentration) cell.paint(region, arb::init_int_concentration{ion, data.init_int_concentration.value()});
            if (data.init_ext_concentration) cell.paint(region, arb::init_ext_concentration{ion, data.init_ext_concentration.value()});
            if (data.init_reversal_potential) cell.paint(region, arb::init_reversal_potential{ion, data.init_reversal_potential.value()});
        }
        for (const auto& [mech, data]: region_data.mechanisms) {
            cell.paint(arb::reg::named(region), mech);
        }
    }

    for (const auto& [location, iclamp]: p.sim.stimuli) {
        cell.place(location, iclamp);
    }
    // Derive model from cell
    auto model = single_cell_model(cell);
    for (const auto& probe: p.sim.probes) {
        model.probe(probe.variable, probe.location, probe.frequency);
    }
    log_info("Deriving simulation: complete");
    return model;
}