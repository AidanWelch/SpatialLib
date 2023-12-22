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
#include <cstdint>
#include <type_traits>
#include <vector>

namespace spatial_lib {

// This constraint requires that coordinates is a vector
template <typename T> concept KDTreeVectorDataConstraint = requires( T t ) {
	requires std::same_as<
		decltype( t.coordinates ),
		std::vector<typename decltype( t.coordinates )::value_type>>;
};

template <typename T> concept KDTreeArrayDataConstraint =
	requires( T t ) { requires std::is_array_v<decltype( t.coordinates )>; };

template <typename T>
	requires KDTreeVectorDataConstraint<T> || KDTreeArrayDataConstraint<T>
class KD_Tree_Base {
	protected:
	using CoordinatesType = decltype( T::coordinates );

	struct Node {
		Node* left;
		Node* right;
		T* data;
	};

	std::vector<Node> nodes; // NOLINT misc-non-private-member-variables-in-classes
	Node* root = nullptr; // NOLINT misc-non-private-member-variables-in-classes

	virtual Node* get_node_from_presorted_dimensions( std::size_t depth, std::size_t index ) = 0;
	virtual void
		presort_dimensions_and_push_nodes( std::vector<T>* data_vector, std::size_t data_size ) = 0;

	void link_tree(
		const std::size_t start,
		const std::size_t end,
		const std::size_t depth,
		Node*& tree_place
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

	public:
	explicit KD_Tree_Base( std::vector<T>& data_vector ){};

	void balance_tree( std::vector<T>& data_vector ) { balance_tree( &data_vector ); }

	void balance_tree( std::vector<T>* data_vector = nullptr ) {
		std::size_t total_size =
			( data_vector == nullptr ) ? nodes.size() : nodes.size() + data_vector->size();

		// reserve the space for all the data in the presorted dimensions
		nodes.reserve( total_size );
		presort_dimensions_and_push_nodes( data_vector, total_size );
		link_tree(0, total_size, 0, root);
	}

	Node* nearest_neighbor( /* const std::vector<CoordinatesType>& coordinates */ ) {
		if ( root == nullptr ) {
			return nullptr;
		}
		return root;
	}
};

template <typename T>
	requires KDTreeVectorDataConstraint<T> || KDTreeArrayDataConstraint<T>
class KD_Tree {};

template <KDTreeVectorDataConstraint T> class KD_Tree<T> : public KD_Tree_Base<T> {
	using CoordinatesType = KD_Tree_Base<T>::CoordinatesType;
	using Node = KD_Tree_Base<T>::Node;
	using KD_Tree_Base<T>::nodes;

	std::size_t dimensions = 0;

	std::vector<std::vector<Node*>> presorted_dimensions;

	inline Node* get_node_from_presorted_dimensions( std::size_t depth, std::size_t index ) final {
		return presorted_dimensions[depth % dimensions][index];
	}

	inline void presort_dimensions_and_push_nodes(
		std::vector<T>* data_vector, std::size_t data_size
	) final {
		// reserve a vector for every dimension
		presorted_dimensions.reserve( dimensions );
		for ( std::size_t i = 0; i < dimensions; i++ ) {
			presorted_dimensions.emplace_back();
			presorted_dimensions[i].reserve( data_size );
		}

		// add existing nodes to the presorted vectors
		for ( Node& node : nodes ) {
			for ( std::vector<Node*>& presorted_dim : presorted_dimensions ) {
				presorted_dim.emplace_back( &node );
			}
		}

		// add the new nodes in the data vector to both nodes and presorteds
		if ( data_vector != nullptr ) {
			for ( T& data : *data_vector ) {
				nodes.emplace_back( nullptr, nullptr, &data );
				for ( std::vector<Node*>& presorted_dim : presorted_dimensions ) {
					presorted_dim.emplace_back( &nodes.back() );
				}
			}
		}

		// actually sort the presorted dimensions
		for ( std::size_t dim = 0; dim < dimensions; dim++ ) {
			std::sort(
				presorted_dimensions[dim].begin(),
				presorted_dimensions[dim].end(),
				[dim]( const Node* node1, const Node* node2 ) {
					return node1->data->coordinates[dim] < node2->data->coordinates[dim];
				}
			);
		}
	}

	public:
	explicit KD_Tree( std::vector<T>& data_vector )
		: KD_Tree_Base<T>( data_vector ), dimensions( data_vector[0].coordinates.size() ) {
		KD_Tree_Base<T>::balance_tree( &data_vector );
	};
};

template <KDTreeArrayDataConstraint T> class KD_Tree<T> : public KD_Tree_Base<T> {
	using CoordinatesType = decltype( T::coordinates );
	static constexpr std::size_t dimensions = std::extent_v<CoordinatesType>;
	using Node = KD_Tree_Base<T>::Node;
	using KD_Tree_Base<T>::nodes;

	std::array<std::vector<Node*>, dimensions> presorted_dimensions;

	inline Node* get_node_from_presorted_dimensions( std::size_t depth, std::size_t index ) final {
		return presorted_dimensions[depth % dimensions][index];
	}

	// this can probably be mostly shared in base
	inline void presort_dimensions_and_push_nodes(
		std::vector<T>* data_vector, std::size_t data_size
	) final {
		// reserve the space for all the data in the presorted dimensions
		for ( std::vector<Node*>& presorted_dim : presorted_dimensions ) {
			presorted_dim.reserve( data_size );
		}

		// add existing nodes to the presorted vectors
		for ( Node& node : nodes ) {
			for ( std::vector<Node*>& presorted_dim : presorted_dimensions ) {
				presorted_dim.emplace_back( &node );
			}
		}

		// add the new nodes in the data vector to both nodes and presorteds
		if ( data_vector != nullptr ) {
			for ( T& data : *data_vector ) {
				nodes.emplace_back( nullptr, nullptr, &data );
				for ( std::vector<Node*>& presorted_dim : presorted_dimensions ) {
					presorted_dim.emplace_back( &nodes.back() );
				}
			}
		}

		// actually sort the presorted dimensions
		for ( std::size_t dim = 0; dim < dimensions; dim++ ) {
			std::sort(
				presorted_dimensions[dim].begin(),
				presorted_dimensions[dim].end(),
				[dim]( const Node* node1, const Node* node2 ) {
					return node1->data->coordinates[dim] < node2->data->coordinates[dim];
				}
			);
		}
	}

	public:
	explicit KD_Tree( std::vector<T>& data_vector ) : KD_Tree_Base<T>( data_vector ) {
		KD_Tree_Base<T>::balance_tree( &data_vector );
	};

	Node* link_not_presorted( std::int64_t start, std::int64_t end, const std::size_t depth ) {
		if ( start == end ) {
			return nullptr;
		}

		std::sort(
			nodes.begin() + start,
			nodes.begin() + end,
			[depth]( const Node& node1, const Node& node2 ) {
				return node1.data->coordinates[depth % dimensions] <
					node2.data->coordinates[depth % dimensions];
			}
		);

		const std::int64_t midpoint = start + ( ( end - start ) / 2 );
		Node& median = nodes[static_cast<std::size_t>( midpoint )];

		median.left = link_not_presorted( start, midpoint, depth + 1 );
		median.right = link_not_presorted( midpoint + 1, end, depth + 1 );
		return &median;
	}
};

}  // namespace spatial_lib

#endif