#ifndef KD_TREE_HPP_
#define KD_TREE_HPP_

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

// #include <iostream>

namespace spatial_lib_recursive {

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
	using CoordinatesType = decltype( T::coordinates );

	struct Node {
		Node* left;
		Node* right;
		T* data;
	};

	std::vector<Node> nodes;
};

template <typename T>
	requires KDTreeVectorDataConstraint<T> || KDTreeArrayDataConstraint<T>
class KD_Tree {};

template <KDTreeVectorDataConstraint T> class KD_Tree<T> : KD_Tree_Base<T> {
	using CoordinatesType = decltype( T::coordinates );

	struct Node {
		Node* left;
		Node* right;
		//std::reference_wrapper<T> data;
		T* data;
	};

	std::vector<Node> nodes;

	Node* link_tree(
		const std::size_t start,
		const std::size_t end,
		const std::size_t depth,
		std::vector<std::vector<Node*>>& presorted_dimensions
	) {
		if ( start == end ) {
			return nullptr;
		}
		const std::size_t midpoint = start + ( ( end - start ) / 2 );
		Node* median = presorted_dimensions[depth % dimensions][midpoint];

		median->left = link_tree( start, midpoint, depth + 1, presorted_dimensions );
		median->right = link_tree( midpoint + 1, end, depth + 1, presorted_dimensions );
		return median;
	}

	Node* root = nullptr;
	std::size_t dimensions = 0;

	public:
	explicit KD_Tree( std::vector<T>& data_vector )
		: dimensions( data_vector[0].coordinates.size() ) {
		balance_tree( &data_vector );
		/*
			std::cout << dimensions << "\n";
			std::cout << nodes.size() << "\n";
			std::cout << root->data->number << "\n";
			std::cout << max_depth << '\n';
			std::cout << null_count << '\n';
			*/
	};

	void balance_tree( std::vector<T>& data_vector ) { 
		balance_tree( &data_vector );
	}

	void balance_tree( std::vector<T>* data_vector = nullptr ) {
		if (data_vector != nullptr && !data_vector->empty()) {
			dimensions = (*data_vector)[0].coordinates.size();
		}
		std::size_t data_size =
			( data_vector == nullptr ) ? nodes.size() : nodes.size() + data_vector->size();

		// reserve the space for all the data in the presorted dimensions
		nodes.reserve( data_size );
		std::vector<std::vector<Node*>> presorted_dimensions;
		presorted_dimensions.reserve( dimensions );
		for ( std::size_t i = 0; i < dimensions; i++ ) {
			presorted_dimensions.emplace_back();
			std::vector<Node*>& presorted_dim = presorted_dimensions[i];
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

		root = link_tree( 0, nodes.size(), 0, presorted_dimensions );
	}

	T* nearest_neighbor( /* const std::vector<CoordinatesType>& coordinates */ ) {
		if ( root == nullptr ) {
			return nullptr;
		}
		return root;
	}
};

template <KDTreeArrayDataConstraint T> class KD_Tree<T> {
	using CoordinatesType = decltype( T::coordinates );
	static constexpr std::size_t dimensions = std::extent_v<CoordinatesType>;

	struct Node {
		Node* left;
		Node* right;
		T* data;
	};
	std::vector<Node> nodes;
	//std::array<std::vector<Node*>, dimensions> presorted_dimensions;

	void link_tree(
		const std::size_t start,
		const std::size_t end,
		const std::size_t depth,
		std::array<std::vector<Node*>, dimensions>& presorted_dimensions,
		Node*& tree_place
	) {

		if ( start == end ) {
			tree_place = nullptr;
			return;
		}

		const std::size_t midpoint = start + ( ( end - start ) / 2 );
		tree_place = presorted_dimensions[depth % dimensions][midpoint];

		link_tree( start, midpoint, depth + 1, presorted_dimensions, tree_place->left );
		link_tree( midpoint + 1, end, depth + 1, presorted_dimensions, tree_place->right );
	}

	Node* root = nullptr;

	public:
	explicit KD_Tree( std::vector<T>& data_vector ) {
		balance_tree( &data_vector );
		// nodes.reserve( data_vector.size() );
		// for ( T& data : data_vector ) {
		// 	nodes.emplace_back( nullptr, nullptr, &data );
		// }
		// root = link_not_presorted( 0, static_cast<std::int64_t>(data_vector.size()), 0 );
		/*
			std::cout << dimensions << "\n";
			std::cout << nodes.size() << "\n";
			std::cout << root->data->number << "\n";
			std::cout << max_depth << '\n';
			std::cout << null_count << '\n';
			*/
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

	void balance_tree( std::vector<T>& data_vector ) { balance_tree( &data_vector ); }

	void balance_tree( std::vector<T>* data_vector = nullptr ) {
		std::size_t data_size =
			( data_vector == nullptr ) ? nodes.size() : nodes.size() + data_vector->size();

		// reserve the space for all the data in the presorted dimensions
		nodes.reserve( data_size );
		std::array<std::vector<Node*>, dimensions> presorted_dimensions;
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

		link_tree( 0, nodes.size(), 0, presorted_dimensions, root );
		//for ( std::vector<Node*>& presorted_dim : presorted_dimensions ) {
		//	presorted_dim.clear();
		//}
	}

	Node* nearest_neighbor( /* const std::array<CoordinatesType, dimensions>& coordinates */ ) {
		if ( root == nullptr ) {
			return nullptr;
		}
		return root;
	}
};

}  // namespace spatial_lib_recursive

#endif