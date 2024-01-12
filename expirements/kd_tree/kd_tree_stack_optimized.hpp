#ifndef KD_TREE_STACK_OPTIMIZED_HPP_
#define KD_TREE_STACK_OPTIMIZED_HPP_

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace spatial_lib_stack_optimized {

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

	std::vector<Node> nodes;  // NOLINT misc-non-private-member-variables-in-classes

	virtual Node* get_node_from_presorted_dimensions( std::size_t depth, std::size_t index ) = 0;
	virtual void
		presort_dimensions_and_push_nodes( std::vector<T>* data_vector, std::size_t data_size ) = 0;

	Node* root = nullptr;  // NOLINT misc-non-private-member-variables-in-classes

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

	public:
	void balance_tree( std::vector<T>& data_vector ) { balance_tree( &data_vector ); }

	void balance_tree( std::vector<T>* data_vector = nullptr ) {
		std::size_t data_size =
			( data_vector == nullptr ) ? nodes.size() : nodes.size() + data_vector->size();

		// reserve the space for all the data in the presorted dimensions
		nodes.reserve( data_size );
		presort_dimensions_and_push_nodes( data_vector, data_size );
		link_tree();
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
		if ( data_vector != nullptr && !data_vector->empty() ) {
			dimensions = ( *data_vector )[0].coordinates.size();
		}
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
		: dimensions( data_vector[0].coordinates.size() ) {
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
	explicit KD_Tree( std::vector<T>& data_vector ) {
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

}  // namespace spatial_lib_stack_optimized

#endif