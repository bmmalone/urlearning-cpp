/* 
 * File:   dynamic_bitset_hash.h
 * Author: malone
 *
 * Created on October 18, 2012, 10:18 PM
 */

#ifndef DYNAMIC_BITSET_HASH_H
#define	DYNAMIC_BITSET_HASH_H

#include <boost/functional/hash.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/serialization/vector.hpp>

// make sure we can use dynamic_bitsets as values in the hash table
namespace boost {
    template <typename B, typename A>
    std::size_t hash_value(const boost::dynamic_bitset<B, A>& bs) {            
        return boost::hash_value(bs.to_ulong());
    }

    // copy-and-pasted from: http://stackoverflow.com/questions/31006792

    namespace serialization {

    template <typename Ar, typename Block, typename Alloc>
        void save(Ar& ar, dynamic_bitset<Block, Alloc> const& bs, unsigned) {
            size_t num_bits = bs.size();
            std::vector<Block> blocks(bs.num_blocks());
            to_block_range(bs, blocks.begin());

            ar & num_bits & blocks;
        }

    template <typename Ar, typename Block, typename Alloc>
        void load(Ar& ar, dynamic_bitset<Block, Alloc>& bs, unsigned) {
            size_t num_bits;
            std::vector<Block> blocks;
            ar & num_bits & blocks;

            bs.resize(num_bits);
            from_block_range(blocks.begin(), blocks.end(), bs);
            bs.resize(num_bits);
        }

    template <typename Ar, typename Block, typename Alloc>
        void serialize(Ar& ar, dynamic_bitset<Block, Alloc>& bs, unsigned version) {
            split_free(ar, bs, version);
        }

} }

//template <typename B, typename A>
struct hasher
{
  bool operator()(const boost::dynamic_bitset<>& bs) const {
        return boost::hash_value(bs.to_ulong());
    }
};

//template <typename B, typename A>
struct key_eq
{
  bool operator()(const boost::dynamic_bitset<>& bs1, const boost::dynamic_bitset<>& bs2) const {
        return (bs1 == bs2);
    }
};


#endif	/* DYNAMIC_BITSET_HASH_H */

