/**
 * @file btree_node.h
 * @brief The header file of btree_node.
 *
 * This file defines class btree_node.
 */

#ifndef sax_btree_node_H_
#define sax_btree_node_H_

#include "btree_header.h"
#include <sax/sysutil.h>
#include <assert.h>


namespace sax {


/**
 *
 * \brief  sdb_node represents a node(internal node or leaf node)
 * in the B tree structure.
 *
 *
 * SDB/btree is made up of a collection of nodes. A node
 * contains information about which of its parent's children
 * is, how many objects it has, whether or not it's a leaf,
 * where it lives on the disk, whether or not it's actually
 * been loaded from the disk, a collection of records(key/value pair),
 * and (if it's not a leaf) a collection of children.
 * It also contains a ptr to its parent.
 *
 * A node page in file is as follows:
 *
 * Page format :
 * @code
 *  +------------------------------+
 *  | leafFlag     | objCount      |
 *  +--------+---------------------+
 *  |childAddress|--->|ChildAddress|
 *  +--------+-----+----+----------+
 *  |DbObj/key | ----- DbObj/value |
 *  +------------+--------+--------+
 *
 *  where DbObj wraps the content of key or value in form of
 *
 *  @DbObj
 *  +-------------+
 *  | size | char*|
 *  +-------------+
 *
 */

typedef unsigned char byte;

enum CChildPos
{
	CCP_INTHIS,
	CCP_INLEFT,
	CCP_INRIGHT,
	CCP_NONE,
};

typedef std::pair<size_t, CChildPos> KEYPOS;

typedef int (cmpfunc)(const void *key1, size_t s1, const void *key2, size_t s2);
class btree_node {
	typedef std::pair<btree_node*, size_t> NodeKeyLocn;
public:
	/**
	 * \brief constructor
	 *
	 *  Constructor initialises everything to its default value.
	 * Not that we assume that the node is a leaf,
	 * and it is not loaded from the disk.
	 */
	btree_node(CbFileHeader& fileHeader, g_share_t* fileLock, size_t& activeNum, cmpfunc * comp);

	~btree_node() {
		unload();
		_clear_kv();
		if (children) 
		{
			for (size_t i=0; i<_fh.maxKeys+1; i++){
				if(children[i])
				{
					delete children[i];
					children[i] = NULL;
				}
			}
			delete [] children;
		}
		if(keys)
			delete [] keys;
		if(values)
			delete [] values;

	}
	
	void _clear_kv()
	{
		if(keys)
		{
			for(size_t i = 0 ; i < _fh.maxKeys; ++i)
			{
				if(keys[i]) 
				{
					delete [] keys[i];
					keys[i] = NULL;
				}
			}
			
			//delete [] keys;
			//keys = NULL;
		}
		if(values)
		{
			for(size_t i = 0 ; i < _fh.maxKeys; ++i)
			{
				if(values[i]) 
				{
					delete [] values[i];
					values[i] = NULL;
				}
			}
			//delete [] values;
			//values = NULL;
		}
	}

	/**
	 * 	\brief when we want to access to node, we should load it to memory from disk.
	 *
	 *  Load a child node from the disk. This requires that we
	 *  have the filepos already in place.
	 */
	btree_node* loadChild(size_t childNum, FILE* f)
	{
		btree_node* child;
		child = children[childNum];
		if (isLeaf || child == 0) {
			return NULL;
		}
		child->childNo = childNum;
		child->parent = this;
		if (child && !child->isLoaded) {
			child->read(f);
		}
		return child;
	}

	/**
	 * \brief delete  all its children and release self memory
	 */
	void unload();

	/**
	 *  \brief release most self memory without unloading is child, used for sdb_btree/_merge method.
	 */
	void unloadself();

	/**
	 * 	\brief read the node from disk.
	 */
	bool read(FILE* f);
	/**
	 * 	\brief write the node to  disk.
	 */
	bool write(FILE* f);
	/**
	 *
	 *  \brief delete a child from a given node.
	 */
	bool delFromLeaf(size_t objNo);

	/**
	 * 	\brief  Find the position of the object in a node.
	 *
	 *  If the key is at current node,
	 * the function returns (pos, ECP_INTHIS). If the key is in a child to
	 * the left of pos, the function returns (pos, ECP_INLEFT). If the node
	 * is an internal node, the function returns (objCount, ECP_INRIGHT).
	 * Otherwise, the function returns ((size_t)-1, false).
	 *
	 * The main assumption here is that we won't be searching for a key
	 *  in this node unless it (a) is not in the tree, or (b) it is in the
	 * subtree rooted at this node.
	 */
	KEYPOS findPos(const void *key, size_t klen);

	/**
	 * 	 \brief Find the position of the nearest object in a node,  given the key.
	 */
	KEYPOS findPos1(const void *key, size_t klen);

	/**
	 * \brief
	 *
	 * We need to change the number of Objects and children, when we split a
	 * 	node or merge two nodes.
	 */
	inline void setCount(size_t newSize) {
		objCount = newSize;
	}
	/**
	 *  \brief it marks this node is dirty
	 *
	 *  only dirty page will be write back to disk.
	 */
	void setDirty(bool is) {
		isDirty = is;
	}

	/**
	 * 	\brief print the shape of  tree.

	 *   eg. below is an example of display result of a btree with 14 nodes,with
	 *   the root node is "continued".
	 *
	 *
	 *	accelerating
	 *	alleviating
	 *	----|bahia
	 *	cocoa
	 *	----|----|continued
	 *	drought
	 *	early
	 *	----|howers
	 *	in
	 *	since
	 *	----|the
	 *	throughout
	 *	week
	 *
	 */
	void display(std::ostream& os = std::cout) {

		size_t i;
		for (i=0; i<objCount; i++) {
			if ( !isLeaf) {
				if (children && children[i])
				children[i]->display(os);
				os<<"----|";
			}
			size_t pfos=0;
			//if (parent)
			//	pfos = parent->fpos;
			os<<"("<<fpos<<" parent="<<pfos<<" isLeaf="<<isLeaf<<" childNo="
			<<childNo<<" objCount="<<objCount<<" isLoaded="<<isLoaded
			<<")"<<endl;
			//os<<"("<<isDirty<<" "<<parent<<" "<<this<<")";
			os<<endl;
		}
		if (!isLeaf) {
			if (children[i])
			children[i]->display(os);
			os<<"----|";
		}
	}

public:
	size_t objCount; //the number of the objects, and if the node is not leafnode,
	long fpos; //its streamoff in file.
	bool isLeaf; //leafNode of internal node, node that int disk we put the data beside the key.
	bool isLoaded; //if it is loaded from disk.
	int isDirty;
	size_t childNo; //from 0 to objCount. Default is size_t(-1).
	btree_node* parent;

	char** keys;
	char** values;
	btree_node** children;
public:
	// size_t activeNodeNum;//It is used for cache mechanism.

private:
	CbFileHeader& _fh;
	size_t& _maxKeys;
	size_t& _pageSize;
	size_t& _overFlowSize;
	g_share_t *_fileLock; //inclusive lock for I/O
	size_t &activeNodeNum;
	cmpfunc * _comp;
private:
	//when overflowing occured, we will allocate serverall sequential
	//overflow page at the end of file.
	long _overflowAddress;
	size_t _overflowPageCount;
};

}
#endif
