#ifndef AUTON_SELECTOR_H
#define AUTON_SELECTOR_H

#include "json.h"
#include <set>
#include "../main.h"
namespace auton_selector {

// Forward declaration
class SelectionNode;

class Selector {
private:
    std::map<std::string, SelectionNode *> nodes; // All nodes indexed by their key
    std::set<std::string> final_options; // Store final options
    std::map<std::set<std::string>, std::function<void()>> function_map; // Map of dependencies to functions
    nlohmann::json json_data_l;
    pros::Controller* master_l;

public:
    Selector(const char* json_data, pros::Controller &controller);
    ~Selector();

    void loadSelectionTree(); // Load selection options from JSON
    void selectChoices(const SelectionNode *node); // Handle user selections
    void startSelection(const std::vector<std::string> &order); // Start the selection process
    void confirmAndRun(); // Confirm and execute the final program
    void map_function(std::set<std::string> dependents, std::function<void()> function); // Map a function to a set of dependent options
    void handle_set_line(int line, const std::string &content);
};

class SelectionNode {
public:
    std::string identifier;
    std::vector<std::string> options; // Options as a vector

    SelectionNode(std::string id, const std::vector<std::string> &opts);
};

// Utility function to handle displaying text on the controller
};
#endif //JSON_CONFIG_H
