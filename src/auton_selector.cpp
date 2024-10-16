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
            for (auto& option: options) {
                all_options[option]=1;
            }
            nodes[key] = new SelectionNode(value["id"], options); // Store current node
        }

        // Load the order from the JSON
        order = json_data_l["order"].get<std::vector<std::string>>();
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
    void Selector::startSelection() {
        for (const auto &key: order) {
            selectChoices(nodes[key]);
        }
    }

    void Selector::ask_selection(std::string choice) {
        selectChoices(nodes[choice]);
    }

    void Selector::endSelection() {
        confirmSelections();
    }

void Selector::confirmSelections() {
    master_l->clear();

    // Prepare to display all options
    std::vector<std::string> formatted_options;

    for (const auto& option : final_options) {
        formatted_options.push_back(option); // No truncation, store the full option
    }

    // Initialize strings for the lines
    std::string line1, line2;
    int current_length = 0;

    // Fill line 1 with options
    for (size_t i = 0; i < formatted_options.size(); ++i) {
        std::string option = formatted_options[i];
        int option_length = option.length();

        // Check if adding this option exceeds the line limit
        if (current_length + option_length + (current_length > 0 ? 2 : 0) <= 15) {
            if (current_length > 0) line1 += "  "; // Add spacing if not the first option
            line1 += option;
            current_length += option_length + (current_length > 0 ? 2 : 0); // Update length
        } else {
            // Move to line 2 if line 1 is full
            if (line2.length() + option_length + (line2.length() > 0 ? 2 : 0) <= 15) {
                if (line2.length() > 0) line2 += "  "; // Add spacing if not the first option
                line2 += option;
            }
        }
    }

    // Display the lines
    handle_set_line(0, line1); // Display the first line
    handle_set_line(1, line2); // Display the second line

    // Confirmation options on the last line
    std::string confirmation_line = "[B] NO [A] YES";
    handle_set_line(2, confirmation_line); // Display confirmation options

    // Confirmation input loop
    while (true) {
        if (master_l->get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B)) {
            // Restart selection process
            final_options.clear(); // Clear previous selections
            startSelection(); // Restart the selection process
            return; // Exit confirmation
        }

        if (master_l->get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A)) {
            // Confirm selections and call associated function
            handle_set_line(2, "Confirmed. Await.");
            return; // Exit confirmation
        }
    }
}

    // Confirm and display the final options
    void Selector::run() {
        function_map[final_options]();
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

    bool Selector::check() const {
        for (const auto& pair : function_map) {
            const auto& dependents = pair.first;

            // Check if each dependent is in the valid options
            for (const auto& dep : dependents) {
                // Check if the dependent exists in all_options
                if (all_options.find(dep) == all_options.end()) {
                    std::cerr << "Invalid dependent: " << dep << " not in valid options." << std::endl;
                    return false; // Return false if any dependent is invalid
                }
            }
        }
        return true; // All checks passed
    }

    bool Selector::is_option_selected(const std::set<std::string>& options) const {
        for (const auto& option : options) {
            if (final_options.find(option) != final_options.end()) {
                return true; // Return true if any option is found
            }
        }
        return false; // Return false if none of the options are found
    }

};