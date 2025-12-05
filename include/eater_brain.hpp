#ifndef EATER_BRAIN_HPP
#define EATER_BRAIN_HPP

#include <vector>
#include <cstdlib>
#include <algorithm>
#include <memory>
#include <unordered_map>

// EaterBrain is a simple node graph.
// Nodes store input and output registers.
// Update does a full input pass (mean of input nodes' outputs) then a full output pass (bernoulli using input as probability).
class EaterBrain {
public:
    enum class NodeType {
        Input,
        Output,
        Hidden,
        Inverted
    };

    struct Node {
        NodeType type;
        std::vector<Node*> input_nodes;
        float input_register = 0.0f;
        float output_register = 0.0f;
    };

    EaterBrain(size_t input_count, size_t output_count);
    EaterBrain(const EaterBrain& other);
    EaterBrain& operator=(const EaterBrain& other);
    EaterBrain(EaterBrain&&) = default;
    EaterBrain& operator=(EaterBrain&&) = default;
    ~EaterBrain() = default;

    size_t get_input_count() const { return input_count; }
    size_t get_output_count() const { return output_count; }

    // Set the output register of an input node (0-based among inputs).
    void set_input(size_t input_index, float value);

    // Advance the brain: first update input registers for all nodes, then output registers.
    void update();

    // Read output register of an output node (0-based among outputs).
    float read_output(size_t output_index) const;

    // Read input register of an output node (0-based among outputs).
    float read_output_input_register(size_t output_index) const;

    // Mutation entry point. Probabilities are checked independently.
    void mutate(float add_node_probability, float remove_node_probability, float add_connection_probability, float remove_connection_probability);

    const std::vector<std::unique_ptr<Node>>& get_nodes() const { return nodes; }

private:
    std::vector<std::unique_ptr<Node>> nodes;
    size_t input_count;
    size_t output_count;

    float random_unit() const { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); }
    float clamp01(float v) const { return std::clamp(v, 0.0f, 1.0f); }
    bool has_hidden_nodes() const;
    void add_hidden_node(NodeType type = NodeType::Hidden);
    void remove_random_hidden_node();
    void add_random_connection();
    void remove_random_connection();
    size_t random_node_index() const;
    void clone_from(const EaterBrain& other);
};

#endif
