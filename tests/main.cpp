#include <array>
#include <iostream>
#include <memory>
#include <vector>
#include "../kd_tree.hpp"

struct Value {
	std::array<int, 4> coordinates;
	int x;
};

int main() {
	std::cout << "Hello World\n" << std::flush;
	std::shared_ptr<std::vector<Value>> smart_data = std::make_shared<std::vector<Value>>();
	auto smart_tree = spatial_lib::KD_Tree(smart_data);
	std::vector<Value> value_data;
	auto value_tree = spatial_lib::KD_Tree(std::move(value_data));
	return 0;
}