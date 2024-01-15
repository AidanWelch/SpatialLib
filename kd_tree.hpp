///////////////////////////////////////////////////////////////////////////////
/* Copyright (c) <2023> <Aidan Welch>

Permission is hereby granted, free of charge, to any person (except as 
specified below) obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom 
the Software is furnished to do so, subject to the following conditions:

This permission IS NOT granted for use by or distribution to entities within
any or all of the following categories:
	- Annual Revenue in any year since 2020 exceeding $250,000 US Dollars.
	- Government Entities
	- Total funding from any government entity exceeding $10,000 US Dollars.
	- Political Action Committees
	- Received any funding from a Political Action Committee.

Entities meeting these categories should contact the copyright holder for
licensing at: aidan@freedwave.com

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software. The notice should be clearly
accessible to end users.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */
///////////////////////////////////////////////////////////////////////////////

#ifndef SPATIAL_LIB_KD_TREE_HPP_
#define SPATIAL_LIB_KD_TREE_HPP_

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <vector>
#include <execution>

namespace spatial_lib {

namespace kd_tree_types {

template <typename Container> concept IsContainer = requires( Container container ) {
	{ container.begin() } -> std::same_as<typename Container::iterator>;
	{ container.end() } -> std::same_as<typename Container::iterator>;
} || std::is_array_v<Container>;

template <typename T> concept IsValidCoordinates =
	IsContainer<T> && requires( T a, T b, std::size_t index ) {
		{ a[index] < b[index] } -> std::convertible_to<bool>;
	};

template <typename T> concept IsValidDataType = IsValidCoordinates<decltype( T::coordinates )>;

template <typename Container> concept InputContainsValidDataTypeNotCArray =
	IsValidDataType<typename Container::value_type>;

template <typename Container> concept InputCArrayContainsValidDataType =
	std::is_array_v<Container> && IsValidDataType<std::remove_all_extents_t<Container>>;

template <typename Container> concept IsDynamicContainer = requires( Container container ) {
	{ container.push_back( Container::value_type ) } -> std::same_as<void>;
};

template <typename Container> concept IsValidInput = IsContainer<Container> &&
	( InputCArrayContainsValidDataType<Container> ||
	  InputContainsValidDataTypeNotCArray<Container> );

template <typename Container> concept IsStaticContainer =
	IsContainer<Container> && ( !IsDynamicContainer<Container> );
template <typename Container> concept IsCArray = std::is_array_v<Container>;
template <typename Container> concept IsStaticContainerNotCArray =
	IsStaticContainer<Container> && ( !IsCArray<Container> );

template <typename T> constexpr const void* staticSize = nullptr;

template <IsStaticContainerNotCArray T> constexpr std::size_t staticSize<T> = std::tuple_size_v<T>;

template <IsCArray T> constexpr std::size_t staticSize<T> = std::extent_v<T>;

template <typename Container> concept InputContainsStaticCoordinates =
	(IsCArray<Container> &&
	 IsStaticContainer<decltype( std::remove_all_extents_t<Container>::coordinates )>) ||
	IsStaticContainer<decltype( Container::value_type::coordinates )>;

template <typename T> constexpr const void* staticDimensions = nullptr;

template <typename Container> concept InputCArrayContainsStaticCoordinates =
	InputCArrayContainsValidDataType<Container> && InputContainsStaticCoordinates<Container>;
template <typename Container> concept InputContainsStaticCoordinatesNotCArray =
	InputContainsValidDataTypeNotCArray<Container> && InputContainsStaticCoordinates<Container>;

template <InputCArrayContainsStaticCoordinates T>
constexpr std::size_t staticDimensions<T> =
	staticSize<decltype( std::remove_all_extents_t<T>::coordinates )>;

template <InputContainsStaticCoordinatesNotCArray T>
constexpr std::size_t staticDimensions<T> = staticSize<decltype( T::value_type::coordinates )>;

}  // namespace kd_tree_types

template <kd_tree_types::IsValidInput Input> class KD_Tree {

	Input input_data;

	using DataType = std::conditional_t<
		std::is_array_v<Input>,
		std::remove_all_extents_t<Input>,
		typename Input::value_type>;

	struct Node {
		Node* left;
		Node* right;
		DataType* data;
	};

	using PresortedContainer = std::conditional_t<
		kd_tree_types::InputContainsStaticCoordinates<Input>,
		std::array<std::vector<Node*>, kd_tree_types::staticDimensions<Input>>,
		std::vector<std::vector<Node*>>>;

	std::vector<Node> nodes;

	Node* root = nullptr;

	std::size_t dimensions = 0;

	PresortedContainer presorted_dimensions;

	void link_tree(
		const std::size_t start, const std::size_t end, const std::size_t depth, Node*& tree_place
	) {

		if ( start == end ) {
			tree_place = nullptr;
			return;
		}

		const std::size_t midpoint = start + ( ( end - start ) / 2 );
		tree_place = get_node_from_presorted_dimensions( depth, midpoint );

		link_tree( start, midpoint, depth + 1, tree_place->left );
		link_tree( midpoint + 1, end, depth + 1, tree_place->right );
	}

	inline void presort_dimension( std::size_t dim ) {
		std::sort(
			std::execution::par_unseq,
			presorted_dimensions[dim].begin(),
			presorted_dimensions[dim].end(),
			[dim]( const Node* node1, const Node* node2 ) {
				return node1->data->coordinates[dim] < node2->data->coordinates[dim];
			}
		);
	}

	public:
	/// Only pass a pointer to the KD Tree if you're sure that input_data will be preserved
	/// in scope for the lifetime of the KD Tree.
	explicit KD_Tree<Input>( Input* input_data ) { generate_tree( input_data ); };

	/// Passing by value leads to the value being moved, this should only be done to preserve
	/// the input_data if it would otherwise go out of scope.
	explicit KD_Tree<Input>( Input input_data ) : input_data( std::move( input_data ) ) {
		generate_tree( &( this->input_data ) );
	}

	void generate_tree( Input* data_container = nullptr ) {
		if constexpr ( kd_tree_types::InputContainsStaticCoordinates<Input> ) {
			dimensions = kd_tree_types::staticDimensions<Input>;
		} else if ( data_container != nullptr && !data_container->empty() ) {
			dimensions = ( *data_container )[0].coordinates.size();
		}
		std::size_t total_size = nodes.size();
		if constexpr ( std::is_array_v<Input> ) {
			total_size += std::extent_v<Input>;
		} else {
			total_size += data_container->size();
		}

		// reserve the space for all the data in the presorted dimensions
		nodes.reserve( total_size );
		if constexpr ( kd_tree_types::InputContainsStaticCoordinates<Input> ) {
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
		if constexpr ( kd_tree_types::InputContainsStaticCoordinates<Input> ) {
			for ( std::size_t dim = 0; dim < kd_tree_types::staticDimensions<Input>; dim++ ) {
				presort_dimension( dim );
			}
		} else {
			for ( std::size_t dim = 0; dim < dimensions; dim++ ) {
				presort_dimension( dim );
			}
		}
		link_tree( 0, total_size, 0, root );
	}

	Node* nearest_neighbor( /* const std::vector<CoordinatesType>& coordinates */ ) {
		if ( root == nullptr ) {
			return nullptr;
		}
		return root;
	}

	inline Node* get_node_from_presorted_dimensions( std::size_t depth, std::size_t index ) {
		if constexpr ( kd_tree_types::InputContainsStaticCoordinates<Input> ) {
			return presorted_dimensions[depth % kd_tree_types::staticDimensions<Input>][index];
		} else {
			return presorted_dimensions[depth % dimensions][index];
		}
	}
};

}  //  namespace spatial_lib

#endif