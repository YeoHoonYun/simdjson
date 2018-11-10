#pragma once

#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <immintrin.h>
#include <x86intrin.h>
#endif

#include <iostream>
#include <iomanip>
//#define MAX_JSON_BYTES 0xFFFFFF

const u32 MAX_DEPTH = 256;
const u32 DEPTH_SAFETY_MARGIN = 32; // should be power-of-2 as we check this
                                    // with a modulo in our hot stage 3 loop
const u32 START_DEPTH = DEPTH_SAFETY_MARGIN;
const u32 REDLINE_DEPTH = MAX_DEPTH - DEPTH_SAFETY_MARGIN;
const size_t MAX_TAPE_ENTRIES = 127 * 1024;
const size_t MAX_TAPE = MAX_DEPTH * MAX_TAPE_ENTRIES;

// TODO: move this to be more like a real class
struct ParsedJson {
public:
  size_t bytecapacity; // indicates how many bits are meant to be supported by
                       // structurals
  u8 *structurals;
  u32 n_structural_indexes;
  u32 *structural_indexes;

  // grossly overprovisioned
  u64 tape[MAX_TAPE];
  u32 tape_locs[MAX_DEPTH];
  u8 * string_buf;// should be at least bytecapacity
  u8 *current_string_buf_loc;
  u8 * number_buf;// holds either doubles or longs, really // should be at least 4 * bytecapacity
  u8 *current_number_buf_loc;
    
    void init() {
        current_string_buf_loc = string_buf;
        current_number_buf_loc = number_buf;

        for (u32 i = 0; i < MAX_DEPTH; i++) {
            tape_locs[i] = i * MAX_TAPE_ENTRIES;
        }
    }

    void dump_tapes() {
        for (u32 i = 0; i < MAX_DEPTH; i++) {
            u32 start_loc = i * MAX_TAPE_ENTRIES;
            std::cout << " tape section i " << i;
            if (i == START_DEPTH) {
                std::cout << "   (START) ";
            } else if ((i < START_DEPTH) || (i >= REDLINE_DEPTH)) {
                std::cout << " (REDLINE) ";
            } else {
                std::cout << "  (NORMAL) ";
            }

            std::cout << " from: " << start_loc << " to: " << tape_locs[i] << " "
                 << " size: " << (tape_locs[i] - start_loc) << "\n";
            for (u32 j = start_loc; j < tape_locs[i]; j++) {
                if (tape[j]) {
                    std::cout << "j: " << j << " tape[j] char " << (char)(tape[j] >> 56)
                              << " tape[j][0..55]: " << (tape[j] & 0xffffffffffffffULL) << "\n";
                }
            }
        }
    }
    // TODO: will need a way of saving strings that's a bit more encapsulated

    void write_tape(u32 depth, u64 val, u8 c) {
        tape[tape_locs[depth]] = val | (((u64)c) << 56);
        tape_locs[depth]++;
    }

    void write_tape_s64(u32 depth, s64 i) {
        *((s64 *)current_number_buf_loc) = i;
        current_number_buf_loc += 8;
        write_tape(depth, current_number_buf_loc - number_buf, 'l');
    }

    void write_tape_double(u32 depth, double d) {
        *((double *)current_number_buf_loc) = d;
        current_number_buf_loc += 8;
        write_tape(depth, current_number_buf_loc - number_buf, 'd');
    }

    u32 save_loc(u32 depth) {
        return tape_locs[depth];
    }

    void write_saved_loc(u32 saved_loc, u64 val, u8 c) {
        tape[saved_loc] = val | (((u64)c) << 56);
    }

    // public interface
#if 1
    
    struct ParsedJsonHandle {
        ParsedJson & pj;
        u32 depth;
        u32 scope_header; // the start of our current scope that contains our current location
        u32 location;     // our current location on a tape

        ParsedJsonHandle(ParsedJson & pj_) : pj(pj_), depth(0), scope_header(0), location(0) {}
        // OK with default copy constructor as the way to clone the POD structure

        // some placeholder navigation. Will convert over to a more native C++-ish way of doing
        // things once it's working (i.e. ++ and -- operators and get start/end iterators)
        // return true if we can do the navigation, false otherwise
        bool next();                      // valid if we're not at the end of a scope
        bool prev();                      // valid if we're not at the start of a scope
        bool up();                        // valid if we are at depth != 0
        bool down();                      // valid if we're at a [ or { call site; moves us to header of that scope
        void to_start_scope();            // move us to the start of our current scope; always succeeds
        void to_end_scope();              // move us to the start of our current scope; always succeeds

        // these navigation elements move us across scope if need be, so allow us to iterate over
        // everything at a given depth
        bool next_flat();                 // valid if we're not at the end of a tape
        bool prev_flat();                 // valid if we're not at the start of a tape

        void print(std::ostream & os);    // print the thing we're currently pointing at
        u8 get_type();                    // retrieve the character code of what we're looking at: [{"sltfn are the possibilities
        s64 get_s64();                    // get the s64 value at this node; valid only if we're at "s" 
        double get_double();              // get the double value at this node; valid only if we're at "d" 
        char * get_string();              // get the string value at this node; valid only if we're at " 
    };

#endif
};


#ifdef DEBUG
inline void dump256(m256 d, std::string msg) {
  for (u32 i = 0; i < 32; i++) {
    std::cout << std::setw(3) << (int)*(((u8 *)(&d)) + i);
    if (!((i + 1) % 8))
      std::cout << "|";
    else if (!((i + 1) % 4))
      std::cout << ":";
    else
      std::cout << " ";
  }
  std::cout << " " << msg << "\n";
}

// dump bits low to high
inline void dumpbits(u64 v, std::string msg) {
  for (u32 i = 0; i < 64; i++) {
    std::cout << (((v >> (u64)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}

inline void dumpbits32(u32 v, std::string msg) {
  for (u32 i = 0; i < 32; i++) {
    std::cout << (((v >> (u32)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}
#else
#define dump256(a, b) ;
#define dumpbits(a, b) ;
#define dumpbits32(a, b) ;
#endif

// dump bits low to high
inline void dumpbits_always(u64 v, std::string msg) {
  for (u32 i = 0; i < 64; i++) {
    std::cout << (((v >> (u64)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}

inline void dumpbits32_always(u32 v, std::string msg) {
  for (u32 i = 0; i < 32; i++) {
    std::cout << (((v >> (u32)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}
