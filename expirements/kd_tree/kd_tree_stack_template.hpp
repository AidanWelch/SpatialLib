#ifndef KD_TREE_STACK_TEMPLATE_HPP_
#define KD_TREE_STACK_TEMPLATE_HPP_

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <vector>

namespace spatial_lib_stack_template {

namespace kd_tree_types {

template <typename Container> concept IsContainer = requires( Container container ) { 
	{ container.begin() } -> std::same_as<typename Container::iterator>;
    { container.end() } -> std::same_as<typename Container::iterator>;
} || std::is_array_v<Container>;

template <typename T> concept IsValidCoordinates = IsContainer<T> &&
requires(T a, T b, std::size_t index) {
    { a[index] < b[index] } -> std::convertible_to<bool>;
};

template <typename T> concept IsValidDataType = IsValidCoordinates<decltype(T::coordinates)>;

template <typename Container> concept InputContainsValidDataTypeNotCArray = IsValidDataType<typename Container::value_type>;

template <typename Container> concept InputCArrayContainsValidDataType = std::is_array_v<Container> && IsValidDataType<std::remove_all_extents_t<Container>>;

template <typename Container> concept IsDynamicContainer = requires(Container container){
	{ container.push_back(Container::value_type) } -> std::same_as<void>;
};

template <typename Container> concept IsValidInput = IsContainer<Container> && (InputCArrayContainsValidDataType<Container> || InputContainsValidDataTypeNotCArray<Container>);

template <typename Container> concept IsStaticContainer = IsContainer<Container> && (!IsDynamicContainer<Container>);
template <typename Container> concept IsCArray = std::is_array_v<Container>;
template <typename Container> concept IsStaticContainerNotCArray = IsStaticContainer<Container> && (!IsCArray<Container>);

template<typename T>
constexpr const void* staticSize = nullptr; 

template<IsStaticContainerNotCArray T>
constexpr std::size_t staticSize<T> = std::tuple_size_v<T>;

template<IsCArray T>
constexpr std::size_t staticSize<T> = std::extent_v<T>;

template<typename Container> concept InputContainsStaticCoordinates =
	(
		IsCArray<Container> && 
		IsStaticContainer<decltype(std::remove_all_extents_t<Container>::coordinates)>
	) ||
	IsStaticContainer<decltype(Container::value_type::coordinates)>;

template<typename T>
constexpr const void* staticDimensions = nullptr; 

template <typename Container> concept InputCArrayContainsStaticCoordinates = InputCArrayContainsValidDataType<Container> && InputContainsStaticCoordinates<Container>;
template <typename Container> concept InputContainsStaticCoordinatesNotCArray = InputContainsValidDataTypeNotCArray<Container> && InputContainsStaticCoordinates<Container>;

template<InputCArrayContainsStaticCoordinates T>
constexpr std::size_t staticDimensions<T> = staticSize<decltype(std::remove_all_extents_t<T>::coordinates)>;

template<InputContainsStaticCoordinatesNotCArray T>
constexpr std::size_t staticDimensions<T> = staticSize<decltype(T::value_type::coordinates)>;

}  // namespace kd_tree_types

template <kd_tree_types::IsValidInput Input>
class KD_Tree {

	using DataType = std::conditional_t<
		std::is_array_v<Input>,
		std::remove_all_extents_t<Input>,
		typename Input::value_type
	>;

	struct Node {
		Node* left;
		Node* right;
		DataType* data;
	};

	using PresortedContainer = std::conditional_t<
		kd_tree_types::InputContainsStaticCoordinates<Input>,
		std::array<std::vector<Node*>, kd_tree_types::staticDimensions<Input>>,
		std::vector<std::vector<Node*>>
	>;

	std::vector<Node> nodes;

	Node* root = nullptr;

	std::size_t dimensions = 0;

	PresortedContainer presorted_dimensions;

	void link_tree() {
		if ( nodes.empty() ) {
			return;
		}

		struct Bounds {
			std::size_t depth;
			std::size_t start;
			std::size_t end;
			Node** parent_link;
		};

		std::vector<Bounds> next_bounds;
		const std::size_t root_point = nodes.size() / 2;
		root = get_node_from_presorted_dimensions( 0, root_point );
		next_bounds.emplace_back( 1, 0, root_point, &root->left );
		next_bounds.emplace_back( 1, root_point + 1, nodes.size(), &root->right );

		// std::size_t max_stack_size = 0;

		while ( !next_bounds.empty() ) {
			const Bounds bounds = std::move( next_bounds.back() );
			next_bounds.pop_back();
			const std::size_t new_depth = bounds.depth + 1;
			const std::size_t midpoint = bounds.start + ( ( bounds.end - bounds.start ) / 2 );
			Node* median = get_node_from_presorted_dimensions( bounds.depth, midpoint );
			*( bounds.parent_link ) = median;
			if ( bounds.start != midpoint ) {
				next_bounds.emplace_back( new_depth, bounds.start, midpoint, &median->left );
			}
			if ( midpoint + 1 != bounds.end ) {
				next_bounds.emplace_back( new_depth, midpoint + 1, bounds.end, &median->right );
			}

			// if (next_bounds.size() > max_stack_size) {
			// 	max_stack_size = next_bounds.size();
			// }
		}
		// std::cout << "stack max stack size: " << max_stack_size << std::endl;
	}

	inline void presort_dimension (std::size_t dim) {
		std::sort(
			presorted_dimensions[dim].begin(),
			presorted_dimensions[dim].end(),
			[dim]( const Node* node1, const Node* node2 ) {
				return node1->data->coordinates[dim] < node2->data->coordinates[dim];
			}
		);
	}

	public:
	explicit KD_Tree<Input>( Input& data_vector ) {
		generate_tree( data_vector );
	};

	void generate_tree( Input& data_vector ) { 
		generate_tree( &data_vector ); 
	}

	void generate_tree( Input* data_container = nullptr ) {
		if constexpr (kd_tree_types::InputContainsStaticCoordinates<Input>) {
			dimensions = kd_tree_types::staticDimensions<Input>;
		} else if (data_container != nullptr && !data_container->empty()) {
			dimensions = data_container->coordinates.size();
		}
		std::size_t total_size = nodes.size();
		if constexpr (std::is_array_v<Input>) {
			total_size += std::extent_v<Input>;
		} else {
			total_size += data_container->size();
		}

		// reserve the space for all the data in the presorted dimensions
		nodes.reserve( total_size );
		if constexpr (kd_tree_types::InputContainsStaticCoordinates<Input>) {
			for ( std::size_t i = 0; i < kd_tree_types::staticDimensions<Input>; i++ ) {
				presorted_dimensions[i].reserve( total_size );
			}
		} else {
			// reserve a vector for every dimension
			presorted_dimensions.reserve( dimensions );
			for ( std::size_t i = 0; i < dimensions; i++ ) {
				presorted_dimensions.emplace_back();
				presorted_dimensions[i].reserve( total_size );
			}
		}
		
		// add existing nodes to the presorted vectors
		for ( Node& node : nodes ) {
			for ( std::vector<Node*>& presorted_dim : presorted_dimensions ) {
				presorted_dim.emplace_back( &node );
			}
		}

		// add the new nodes in the data vector to both nodes and presorteds
		if ( data_container != nullptr ) {
			for ( DataType& data : *data_container ) {
				nodes.emplace_back( nullptr, nullptr, &data );
				for ( std::vector<Node*>& presorted_dim : presorted_dimensions ) {
					presorted_dim.emplace_back( &nodes.back() );
				}
			}
		}

		// actually sort the presorted dimensions
		if constexpr (kd_tree_types::InputContainsStaticCoordinates<Input>) {
			for ( std::size_t dim = 0; dim < kd_tree_types::staticDimensions<Input>; dim++ ) {
				presort_dimension(dim);
			}
		} else {
			for ( std::size_t dim = 0; dim < dimensions; dim++ ) {
				presort_dimension(dim);
			}
		}
		link_tree();
	}

	Node* nearest_neighbor( /* const std::vector<CoordinatesType>& coordinates */ ) {
		if ( root == nullptr ) {
			return nullptr;
		}
		return root;
	}

	inline Node* get_node_from_presorted_dimensions( std::size_t depth, std::size_t index ) {
		if constexpr (kd_tree_types::InputContainsStaticCoordinates<Input>) {
			return presorted_dimensions[depth % kd_tree_types::staticDimensions<Input>][index];
		} else {
			return presorted_dimensions[depth % dimensions][index];
		}
	}
};

}  // namespace spatial_lib_stack_template

#endif