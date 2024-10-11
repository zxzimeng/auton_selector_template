#include "auton_selector/auton_selector.h"

namespace auton_selector {
    // SelectionNode constructor
    SelectionNode::SelectionNode(std::string id, const std::vector<std::string> &opts)
        : identifier(std::move(id)), options(opts) {
    }

    // Selection constructor
    Selector::Selector(const char* json_data, pros::Controller &controller){
        master_l = &controller;;
        json_data_l = nlohmann::json::parse(json_data);
        loadSelectionTree();
        startSelection(json_data_l["order"]); // Start selection based on the defined order
    }

    // Selection destructor
    Selector::~Selector() {
        for (auto &val: nodes | std::views::values) {
            delete val; // Clean up dynamically allocated nodes
        }
    }

    // Load selection tree from JSON
    void Selector::loadSelectionTree() {
        for (const auto &[key, value]: json_data_l["choices"].items()) {
            std::vector<std::string> options = value["options"].get<std::vector<std::string> >();
            nodes[key] = new SelectionNode(value["id"], options); // Store current node
        }
    }

    // Handle user selections
    void Selector::selectChoices(const SelectionNode *node) {
        if (!node) return;

        master_l->clear();
        int selected = 0;

        // Initial display of options
        handle_set_line(0, "[<] " + node->options[0]); // First option with left indicator
        if (node->options.size() > 1) {
            handle_set_line(1, "[>] " + node->options[1]); // Second option with right indicator
        }
        handle_set_line(2, "[A] " + node->options[0]); // Default selected option

        while (true) {
            if (master_l->get_digital_new_press(pros::E_CONTROLLER_DIGITAL_LEFT)) {
                selected = (selected > 0) ? selected - 1 : 0; // Navigate left
                handle_set_line(2, "[A] " + node->options[selected]); // Update selected line
            }

            if (master_l->get_digital_new_press(pros::E_CONTROLLER_DIGITAL_RIGHT)) {
                selected = (selected < node->options.size() - 1) ? selected + 1 : selected; // Navigate right
                handle_set_line(2, "[A] " + node->options[selected]); // Update selected line
            }

            if (master_l->get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A)) {
                final_options.insert(node->options[selected]); // Store the selected option
                return; // Exit the current selection
            }
        }
    }

    // Start the selection process based on order
    void Selector::startSelection(const std::vector<std::string> &order) {
        for (const auto &key: order) {
            selectChoices(nodes[key]);
        }
        confirmAndRun();
    }

    // Confirm and display the final options
    void Selector::confirmAndRun() {
        function_map[final_options];
    }

    void Selector::map_function(std::set<std::string> dependents, std::function<void()> function) {
        function_map[dependents] = std::move(function);
    }

    // Utility function to handle displaying text on the controller
    void Selector::handle_set_line(int line, const std::string &content) {
        master_l->clear_line(line);
        pros::delay(100);
        master_l->set_text(line, 0, content);
        pros::delay(100);
    }
};