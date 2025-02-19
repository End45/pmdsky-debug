// Shared types used throughout the game

#ifndef HEADERS_TYPES_COMMON_H_
#define HEADERS_TYPES_COMMON_H_

#include "enums.h"
#include "../dungeon_mode/dungeon_mode_common.h"
#include "file_io.h"

// A slice in the usual programming sense: a pointer, length, and capacity.
// Used for the implementation of vsprintf(3), but maybe it's used elsewhere as well.
struct slice {
    void* data;        // Pointer to the data buffer
    uint32_t capacity; // How much space is available in total
    uint32_t length;   // How much space is currently filled
};
ASSERT_SIZE(struct slice, 12);

// Function to append data to a struct slice, and return a success flag.
typedef bool (*slice_append_fn_t)(struct slice* slice, void* data, uint32_t data_len);

// Program position info (basically stack trace info) for debug logging.
struct prog_pos_info {
    char* file; // file name
    int line;   // line number
};
ASSERT_SIZE(struct prog_pos_info, 8);

// Metadata describing a single memory block. A block is a chunk of dynamically allocated memory.
// It can contain nothing, an allocated object, or a memory arena that itself contains blocks.
struct mem_block {
    // 0x0: The type of content in this block (see enum memory_alloc_flag).
    uint32_t f_in_use : 1; // Block is reserved and cannot be split to accomodate new allocations
    uint32_t f_object : 1; // Block contains a normal object
    uint32_t f_arena : 1;  // Block contains a memory arena
    uint32_t content_flags_unused : 29;

    // 0x4: Flags passed internally to the memory allocator when this block was allocated
    // (see enum memory_alloc_flag).
    uint32_t f_alloc_in_use : 1; // Block was reserved when allocated
    uint32_t f_alloc_object : 1; // Block was allocated as an object
    // Block was allocated as a memory arena.
    // f_alloc_arena is always used in tandem with f_alloc_in_use, and influences how the
    // allocator tries to locate a free block of memory to allocate. However, the arena will
    // be for private use only, and will be viewed as in use by the allocator until the arena
    // is freed.
    uint32_t f_alloc_arena : 1;
    // Block was allocated as a subarena.
    // An allocation with f_alloc_subarena is used when creating a new global memory arena,
    // and guarantees that f_arena will be set and that f_in_use will NOT be set, which allows
    // blocks to be carved out when the allocator needs memory.
    uint32_t f_alloc_subarena : 1;
    uint32_t allocator_flags_unused : 28;

    // 0x8: Flags passed by the user to the memory allocator API functions when this block was
    // allocated. The least significant byte are reserved for specifying the memory arena to use,
    // and have functionality determined by the arena locator function currently in use by the
    // game. The upper bytes are the same as the internal memory allocator flags
    // (just left-shifted by 8).
    uint32_t user_flags : 8;
    uint32_t f_user_alloc_in_use : 1;   // See f_alloc_in_use
    uint32_t f_user_alloc_object : 1;   // See f_alloc_object
    uint32_t f_user_alloc_arena : 1;    // See f_alloc_arena
    uint32_t f_user_alloc_subarena : 1; // See f_alloc_subarena
    uint32_t user_flags_unused : 20;

    void* data;         // 0xC: Pointer to the start of the memory block
    uint32_t available; // 0x10: Number of free bytes in the memory block. Always a multiple of 4.
    uint32_t used;      // 0x14: Number of used bytes in the memory block. Always a multiple of 4.
};
ASSERT_SIZE(struct mem_block, 24);

// Metadata for a memory arena. A memory arena is a large, contiguous region of memory that can
// be carved up into chunks by the memory allocator as needed (when dynamic allocations are
// requested). An arena starts with one large, vacant block, which gradually gets subdivided as
// allocations are made within the arena.
struct mem_arena {
    // 0x0: The type of content in this arena, as a bitfield (see enum memory_alloc_flag).
    // Always seems to be 2 (corresponding to MEM_OBJECT), which makes sense since an arena
    // is by definition a region where objects can be allocated.
    uint32_t content_flags;
    // 0x4: Pointer to the parent arena if this is a subarena, or null otherwise.
    struct mem_arena* parent;
    struct mem_block* blocks; // 0x8: Array of memory blocks in the arena
    uint32_t n_blocks;        // 0xC: Number of memory blocks in the arena
    uint32_t max_blocks;      // 0x10: Maximum number of memory blocks the arena can hold
    void* data;               // 0x14: Pointer to the start of the memory arena
    uint32_t len;             // 0x18: Total length of the memory arena. Always a multiple of 4.
};
ASSERT_SIZE(struct mem_arena, 28);

// Global table of all heap allocations
struct mem_alloc_table {
    uint32_t n_arenas;              // 0x0: Number of global memory arenas (including subarenas)
    struct mem_arena default_arena; // 0x4: The default memory arena for allocations
    // Not actually sure how long this array is, but has at least 4 elements, and can't have
    // more than 8 because it would overlap with default_arena.data
    struct mem_arena* arenas[8]; // 0x20: Array of global memory arenas
};
ASSERT_SIZE(struct mem_alloc_table, 64);

// Functions to get the desired memory arena for (de)allocation, or null if there's no preference.
// flags has the same meaning as the flags passed to MemAlloc.
typedef struct mem_arena* (*get_alloc_arena_fn_t)(struct mem_arena* arena, uint32_t flags);
typedef struct mem_arena* (*get_free_arena_fn_t)(struct mem_arena* arena, void* ptr);

struct mem_arena_getters {
    get_alloc_arena_fn_t get_alloc_arena; // Arena to be used by MemAlloc and friends
    get_free_arena_fn_t get_free_arena;   // Arena to be used by MemFree
};
ASSERT_SIZE(struct mem_arena_getters, 8);

// 64-bit signed fixed-point number with 16 fraction bits.
// Represents the number ((upper << 16) + (lower >> 16) + (lower & 0xFFFF) * 2^-16)
struct fx64 {
    int32_t upper;  // sign bit, plus the 31 most significant integer bits
    uint32_t lower; // the 32 least significant bits (16 integer + 16 fraction)
};
ASSERT_SIZE(struct fx64, 8);

struct overlay_load_entry {
    enum overlay_group_id group;
    // These are function pointers, but not sure of the signature.
    void* entrypoint;
    void* destructor;
    void* frame_update; // Possibly?
};
ASSERT_SIZE(struct overlay_load_entry, 16);

// This seems to be a simple structure used with utility functions related to managing items
// in bulk, such as in the player's bag, storage, and Kecleon shops.
struct bulk_item {
    struct item_id_16 id;
    uint16_t quantity; // Definitely in some contexts, but not verified in all
};
ASSERT_SIZE(struct bulk_item, 4);

// Structure for dialog boxes?
struct dialog_box {
    undefined fields_0x0[12];
    undefined* field_0xc; // Some struct pointer
    undefined fields_0xd[208];
};
ASSERT_SIZE(struct dialog_box, 224);

// These flags are shared with the function to display text inside message boxes
// So they might need a rename once more information is found
struct preprocessor_flags {
    uint16_t unknown0 : 13;
    bool show_speaker : 1;
    uint32_t unknown18 : 18;
};
ASSERT_SIZE(struct preprocessor_flags, 4);

// Represents arguments that might be passed to the PreprocessString function
struct preprocessor_args {
    uint32_t flag_vals[4];  // 0x0: These are usually IDs with additional flags attached to them
    uint32_t id_vals[5];    // 0x10
    int32_t number_vals[5]; // 0x24
    char* strings[5];       // 0x38
    // 0x4C: An optional argument that is used to insert the name of a Pokémon
    // When they're talking through a message box. It requires it's respective flag to be on
    uint32_t speaker_id;
};
ASSERT_SIZE(struct preprocessor_args, 80);

// Type matchup table, not including TYPE_NEUTRAL.
// Note that Ghost's immunities seem to be hard-coded elsewhere. In this table, both Normal and
// Fighting are encoded as neutral against Ghost.
//
// Row index corresponds to the attack type and the column index corresponds to the defender type.
// C-style access: type_matchup_table[attack_type][target_type] or
// *(&type_matchup_table[0][0] + attack_type*18 + target_type)
struct type_matchup_table {
    struct type_matchup_16 matchups[18][18];
};
ASSERT_SIZE(struct type_matchup_table, 648);

// Type matchup combinator table, for combining two type matchups into one.
// This table is symmetric, and maps (type_matchup, type_matchup) -> type_matchup
struct type_matchup_combinator_table {
    enum type_matchup combination[4][4];
};
ASSERT_SIZE(struct type_matchup_combinator_table, 64);

// In the move data, the target and range are encoded together in the first byte of a single
// two-byte field. The target is the lower half, and the range is the upper half.
#pragma pack(push, 2)
struct move_target_and_range {
    enum move_target target : 4;
    enum move_range range : 4;
    enum move_ai_condition ai_condition : 4;
    uint16_t unused : 4; // At least I'm pretty sure this is unused...
};
ASSERT_SIZE(struct move_target_and_range, 2);
#pragma pack(pop)

// Data for a single move
struct move_data {
    uint16_t base_power;                          // 0x0
    struct type_id_8 type;                        // 0x2
    struct move_category_8 category;              // 0x3
    struct move_target_and_range target_range;    // 0x4
    struct move_target_and_range ai_target_range; // 0x6: Target/range as seen by the AI
    uint8_t pp;                                   // 0x8
    uint8_t ai_weight; // 0x9: Possibly. Weight for AI's random move selection
    // 0xA: Both accuracy values are used to calculate the move's actual accuracy.
    // See the PMD Info Spreadsheet.
    uint8_t accuracy1;
    uint8_t accuracy2; // 0xB
    // 0xC: If this move has a random chance AI condition (see enum move_ai_condition),
    // this is the chance that the AI will consider a potential target as elegible
    uint8_t ai_condition_random_chance;
    uint8_t strikes;              // 0xD: Number of times the move hits (i.e. for multi-hit moves)
    uint8_t max_ginseng_boost;    // 0xE: Maximum possible Ginseng boost for this move
    uint8_t crit_chance;          // 0xF: The base critical hit chance
    bool reflected_by_magic_coat; // 0x10
    bool can_be_snatched;         // 0x11
    bool fails_while_muzzled;     // 0x12
    // 0x13: Seems to be used by the AI with Status Checker for using moves against frozen monsters
    bool ai_can_use_against_frozen;
    bool usable_while_taunted; // 0x14
    // 0x15: Index in the string files of the range string to be displayed in the move info screen
    uint8_t range_string_idx;
    struct move_id_16 id; // 0x16
    // 0x18: Index in the string files of the message string to be displayed in the dungeon message
    // log when a move is used. E.g., the default (0) is "[User] used [move]!"
    uint16_t message_string_idx;
};
ASSERT_SIZE(struct move_data, 26);

// The move data table, as contained within /BALANCE/waza_p.bin, and when loaded into memory.
struct move_data_table {
    struct move_data entries[559];
};
ASSERT_SIZE(struct move_data_table, 14534);

// Reduced version of dungeon_mode::move that stores less info
// Dungeon mode might also use these entries sometimes
struct ground_move {
    // 0x0: flags: 1-byte bitfield
    // See move::flags0 for details
    bool f_exists : 1;
    bool f_subsequent_in_link_chain : 1;
    bool f_enabled_for_ai : 1;
    bool f_set : 1;
    bool f_last_used : 1; // unconfirmed, but probably the same as struct move
    bool f_disabled : 1;
    uint8_t flags_unk6 : 2;

    undefined field_0x1;  // Probably padding since it doesn't get initialized
    struct move_id_16 id; // 0x2
    uint8_t ginseng;      // 0x4: Ginseng boost
    undefined field_0x5;  // Probably padding since it doesn't get initialized
};
ASSERT_SIZE(struct ground_move, 6);

// Used to store monster data in ground mode
// (Team members in the assembly, guest pokémon, etc.)
// Dungeon mode might also use these entries sometimes
struct ground_monster {
    bool is_valid;                 // 0x0: True if the entry is valid
    int8_t level;                  // 0x1: Monster level
    struct dungeon_id_8 joined_at; // 0x2
    uint8_t joined_at_floor;       // 0x3: See struct monster::joined_at_floor
    struct monster_id_16 id;       // 0x4: Monster ID
    int8_t level_at_first_evo;     // 0x6: Level upon first evolution, or 0 if not applicable
    int8_t level_at_second_evo;    // 0x7: Level upon second evolution, or 0 if not applicable
    uint16_t iq;                   // 0x8
    uint16_t max_hp;               // 0xA
    int8_t atk;                    // 0xC
    int8_t sp_atk;                 // 0xD
    int8_t def;                    // 0xE
    int8_t sp_def;                 // 0xF
    int exp;                       // 0x10
    // 0x14: Bitvector that keeps track of which IQ skills the monster has enabled.
    // See enum iq_skill_id for the meaning of each bit.
    uint32_t iq_skill_flags[3];
    struct tactic_id_8 tactic; // 0x20
    undefined field_0x21;
    struct ground_move moves[4]; // 0x22
    char name[10];               // 0x3A: Display name of the monster
};
ASSERT_SIZE(struct ground_monster, 68);

// Seems to store information about active team members, including those from special episodes.
// A lot of the fields seem to be analogous to fields on struct monster.
struct team_member {
    // 0x0: flags: 1-byte bitfield
    bool f_is_valid : 1;
    uint8_t flags_unk1 : 7;

    bool is_leader;                // 0x1
    uint8_t level;                 // 0x2
    struct dungeon_id_8 joined_at; // 0x3
    uint8_t joined_at_floor;       // 0x4
    undefined field_0x5;
    uint16_t iq;          // 0x6
    int16_t member_index; // 0x8: Index in the list of all team members (not just the active ones)
    int16_t team_index;   // 0xA: In order by team lineup
    struct monster_id_16 id; // 0xC
    uint16_t current_hp;     // 0xE
    uint16_t max_hp;         // 0x10
    int8_t atk;              // 0x12
    int8_t sp_atk;           // 0x13
    int8_t def;              // 0x14
    int8_t sp_def;           // 0x15
    undefined field_0x16;
    undefined field_0x17;
    int exp;              // 0x18
    struct move moves[4]; // 0x1C
    undefined field_0x3C;
    undefined field_0x3D;
    struct item held_item;         // 0x3E
    int16_t belly;                 // 0x44: Integer part
    int16_t belly_thousandths;     // 0x46
    int16_t max_belly;             // 0x48: Integer part
    int16_t max_belly_thousandths; // 0x4A
    // 0x4C: Bitvector that keeps track of which IQ skills the monster has enabled.
    // See enum iq_skill_id for the meaning of each bit.
    uint32_t iq_skill_flags[3];
    struct tactic_id_8 tactic; // 0x58
    undefined field_0x59;
    undefined field_0x5A;
    undefined field_0x5B;
    undefined field_0x5C;
    undefined field_0x5D;
    char name[10]; // 0x5E: Display name of the monster
};
ASSERT_SIZE(struct team_member, 104);

// A common structure for pairs of dungeon/floor values
struct dungeon_floor_pair {
    struct dungeon_id_8 dungeon_id;
    uint8_t floor_id;
};
ASSERT_SIZE(struct dungeon_floor_pair, 2);

// Unknown struct included in the dungeon_init struct (see below)
struct unk_dungeon_init {
    undefined field_0x0;
    undefined field_0x1;
    undefined field_0x2;
    undefined field_0x3;
    undefined field_0x4;
    undefined field_0x5;
    undefined field_0x6;
    undefined field_0x7;
    undefined field_0x8;
    undefined field_0x9;
    undefined field_0xA;
    undefined field_0xB;
    undefined field_0xC;
    undefined field_0xD;
    undefined field_0xE;
    undefined field_0xF;
    undefined field_0x10;
    undefined field_0x11;
    undefined field_0x12;
    undefined field_0x13;
    undefined field_0x14;
    undefined field_0x15;
    undefined field_0x16;
    undefined field_0x17;
    undefined field_0x18;
    undefined field_0x19;
    undefined field_0x1A;
    undefined field_0x1B;
    undefined field_0x1C;
    undefined field_0x1D;
    undefined field_0x1E;
    undefined field_0x1F;
    undefined field_0x20;
    undefined field_0x21;
    undefined field_0x22;
    undefined field_0x23;
    undefined field_0x24;
    undefined field_0x25;
    undefined field_0x26;
    undefined field_0x27;
    undefined field_0x28;
    undefined field_0x29;
    undefined field_0x2A;
    undefined field_0x2B;
    undefined field_0x2C;
    undefined field_0x2D;
    undefined field_0x2E;
    undefined field_0x2F;
    undefined field_0x30;
    undefined field_0x31;
    undefined field_0x32;
    undefined field_0x33;
    undefined field_0x34;
    undefined field_0x35;
    undefined field_0x36;
    undefined field_0x37;
    undefined field_0x38;
    undefined field_0x39;
    undefined field_0x3A;
    undefined field_0x3B;
    undefined field_0x3C;
    undefined field_0x3D;
    undefined field_0x3E;
    undefined field_0x3F;
    undefined field_0x40;
    undefined field_0x41;
    undefined field_0x42;
    undefined field_0x43;
    undefined field_0x44;
    undefined field_0x45;
    undefined field_0x46;
    undefined field_0x47;
    undefined field_0x48;
    undefined field_0x49;
    undefined field_0x4A;
    undefined field_0x4B;
    undefined field_0x4C;
    undefined field_0x4D;
    undefined field_0x4E;
    undefined field_0x4F;
    undefined field_0x50;
    undefined field_0x51;
    undefined field_0x52;
    undefined field_0x53;
    undefined field_0x54;
    undefined field_0x55;
    undefined field_0x56;
    undefined field_0x57;
    undefined field_0x58;
    undefined field_0x59;
    undefined field_0x5A;
    undefined field_0x5B;
    undefined field_0x5C;
    undefined field_0x5D;
    undefined field_0x5E;
    undefined field_0x5F;
    undefined field_0x60;
    undefined field_0x61;
    undefined field_0x62;
    undefined field_0x63;
    undefined field_0x64;
    undefined field_0x65;
    undefined field_0x66;
    undefined field_0x67;
    undefined field_0x68;
    undefined field_0x69;
    undefined field_0x6A;
    undefined field_0x6B;
    undefined field_0x6C;
    undefined field_0x6D;
    undefined field_0x6E;
    undefined field_0x6F;
    undefined field_0x70;
    undefined field_0x71;
    undefined field_0x72;
    undefined field_0x73;
    undefined field_0x74;
    undefined field_0x75;
    undefined field_0x76;
    undefined field_0x77;
    undefined field_0x78;
    undefined field_0x79;
    undefined field_0x7A;
    undefined field_0x7B;
    undefined field_0x7C;
    undefined field_0x7D;
    undefined field_0x7E;
    undefined field_0x7F;
    undefined field_0x80;
    undefined field_0x81;
    undefined field_0x82;
    undefined field_0x83;
    undefined field_0x84;
    undefined field_0x85;
    undefined field_0x86;
    undefined field_0x87;
    undefined field_0x88;
    undefined field_0x89;
    undefined field_0x8A;
    undefined field_0x8B;
    undefined field_0x8C;
    undefined field_0x8D;
    undefined field_0x8E;
    undefined field_0x8F;
    undefined field_0x90;
    undefined field_0x91;
    undefined field_0x92;
    undefined field_0x93;
    undefined field_0x94;
    undefined field_0x95;
    undefined field_0x96;
    undefined field_0x97;
    undefined field_0x98;
    undefined field_0x99;
    undefined field_0x9A;
    undefined field_0x9B;
    undefined field_0x9C;
    undefined field_0x9D;
    undefined field_0x9E;
    undefined field_0x9F;
    undefined field_0xA0;
    undefined field_0xA1;
    undefined field_0xA2;
    undefined field_0xA3;
    undefined field_0xA4;
    undefined field_0xA5;
    undefined field_0xA6;
    undefined field_0xA7;
    undefined field_0xA8;
    undefined field_0xA9;
    undefined field_0xAA;
    undefined field_0xAB;
    undefined field_0xAC;
    undefined field_0xAD;
    undefined field_0xAE;
    undefined field_0xAF;
    undefined field_0xB0;
    undefined field_0xB1;
    undefined field_0xB2;
    undefined field_0xB3;
    undefined field_0xB4;
    undefined field_0xB5;
    undefined field_0xB6;
    undefined field_0xB7;
    undefined field_0xB8;
    undefined field_0xB9;
    undefined field_0xBA;
    undefined field_0xBB;
    undefined field_0xBC;
    undefined field_0xBD;
    undefined field_0xBE;
    undefined field_0xBF;
    undefined field_0xC0;
    undefined field_0xC1;
    undefined field_0xC2;
    undefined field_0xC3;
    undefined field_0xC4;
    undefined field_0xC5;
    undefined field_0xC6;
    undefined field_0xC7;
    undefined field_0xC8;
    undefined field_0xC9;
    undefined field_0xCA;
    undefined field_0xCB;
    undefined field_0xCC;
    undefined field_0xCD;
    undefined field_0xCE;
    undefined field_0xCF;
    undefined field_0xD0;
    undefined field_0xD1;
    undefined field_0xD2;
    undefined field_0xD3;
    undefined field_0xD4;
    undefined field_0xD5;
    undefined field_0xD6;
    undefined field_0xD7;
    undefined field_0xD8;
    undefined field_0xD9;
    undefined field_0xDA;
    undefined field_0xDB;
    undefined field_0xDC;
    undefined field_0xDD;
    undefined field_0xDE;
    undefined field_0xDF;
    undefined field_0xE0;
    undefined field_0xE1;
    undefined field_0xE2;
    undefined field_0xE3;
    undefined field_0xE4;
    undefined field_0xE5;
    undefined field_0xE6;
    undefined field_0xE7;
};
ASSERT_SIZE(struct unk_dungeon_init, 232);

// A struct used to init certain values in the dungeon struct when entering dungeon mode.
// Gets initialized in ground mode.
struct dungeon_init {
    struct dungeon_id_8 id; // 0x0: Copied into dungeon::id
    uint8_t floor;          // 0x1: Copied into dungeon::floor
    undefined2 field_0x2;   // Copied into dungeon::field_0x74C
    undefined field_0x4;
    bool nonstory_flag;      // 0x5: Copied into dungeon::nonstory_flag
    bool recruiting_enabled; // 0x6: Copied into dungeon::recruiting_enabled
    // 0x7: If true, dungeon::recruiting_enabled gets set to false. Overrides recruiting_enabled.
    bool force_disable_recruiting;
    undefined field_0x8;            // Copied into dungeon::field_0x75A
    undefined field_0x9;            // Copied into dungeon::field_0x75B
    bool send_home_disabled;        // 0xA: Copied into dungeon::send_home_disabled
    bool hidden_land_flag;          // 0xB: Copied into dungeon::hidden_land_flag
    bool skip_faint_animation_flag; // 0xC: Copied into dungeon::skip_faint_animation_flag
    // 0xD: Copied into dungeon::dungeon_objective. Read as a signed byte (?).
    struct dungeon_objective_8 dungeon_objective;
    int8_t field_0xE;
    bool has_guest_pokemon; // 0xF: If true, a guest pokémon will be added to your team
    bool send_help_item;    // 0x10: If true, you recive an item at the start of the dungeon
    bool show_rescues_left; // 0x11: If true, you get a message saying how many rescue chances you
                            // have left
    undefined field_0x12;
    undefined field_0x13;
    // 0x14
    // [EU]0x22DF920 loads this as a word
    // [EU]0x22DFBAC loads this as a signed byte
    // ???
    undefined4 field_0x14; // Copied into dungeon::field_0x750
    // Copied into dungeon::field_0x754, and into dungeon::field_0x7A0 during rescues
    undefined4 field_0x18;
    // 0x1C: Array containing the list of quest pokémon that will join the team in the dungeon
    // (max 2)
    struct ground_monster guest_pokemon[2];
    // 0xA4: Used as a base address at [EU]0x22E0354 and [EU]0x22E03AC.
    // It's probably a separate struct.
    undefined field_0xA4;
    undefined field_0xA5;
    undefined field_0xA6;
    undefined field_0xA7;
    // 0xA8: ID of the item to give to the player if send_help_item is true
    struct item_id_16 help_item;
    undefined field_0xAA;
    undefined field_0xAB;
    bool boost_max_money_amount; // 0xAC: Copied into dungeon::boost_max_money_amount
    undefined field_0xAD;
    undefined field_0xAE;
    undefined field_0xAF;
    undefined4 field_0xB0;
    undefined field_0xB4; // Gets set to dungeon::id during dungeon init
    undefined field_0xB5; // Gets set to dungeon::floor during dungeon init
    undefined field_0xB6;
    undefined field_0xB7;
    // 0xB8: Used as a base address at [EU]0x22E0ABC.
    // It's probably a separate struct.
    undefined field_0xB8; // Gets set to dungeon::id during dungeon init
    undefined field_0xB9; // Gets set to dungeon::floor during dungeon init
    undefined field_0xBA;
    undefined field_0xBB;
    undefined4 field_0xBC;
    // 0xC0: Used as a base address at [EU]0x22E0A4C
    struct unk_dungeon_init field_0xC0;
    undefined field_0x1A8;
    // Probably padding, these bytes aren't accessed by the funtion that inits this struct
    undefined field_0x1A9;
    undefined field_0x1AA;
    undefined field_0x1AB;
};
ASSERT_SIZE(struct dungeon_init, 428);

// Unverified, ported from Irdkwia's notes
struct dungeon_unlock_entry {
    struct dungeon_id_8 dungeon_id;
    uint8_t scenario_balance_min;
};
ASSERT_SIZE(struct dungeon_unlock_entry, 2);

// Unverified, ported from Irdkwia's notes
struct dungeon_return_status {
    bool flag;
    uint8_t _padding;
    int16_t string_id;
};
ASSERT_SIZE(struct dungeon_return_status, 4);

// Structure describing various player progress milestones?
// Ported directly from Irdkwia's notes. The only confirmed thing is the struct size.
struct global_progress {
    undefined unk_pokemon_flags1[148];        // 0x0: unused
    undefined field_0x94[4];                  // 0x94
    undefined unk_pokemon_flags2[148];        // 0x98: used
    undefined exclusive_pokemon_flags[23];    // 0x12C: partially used, only for Time/Darkness
    undefined dungeon_max_reached_floor[180]; // 0x143: used
    undefined field_0x1f7;                    // unused
    undefined4 nb_adventures;                 // 0x1F8: used
    undefined field_0x1fc[16];                // unknown/unused
};
ASSERT_SIZE(struct global_progress, 524);

// The adventure log structure.
struct adventure_log {
    uint32_t completion_flags[4];
    uint32_t nb_dungeons_cleared;
    uint32_t nb_friend_rescues;
    uint32_t nb_evolutions;
    uint32_t nb_eggs_hatched;
    uint32_t successful_steals; // Unused in Sky
    uint32_t nb_faints;
    uint32_t nb_victories_on_one_floor;
    uint32_t pokemon_joined_counter;
    uint32_t pokemon_battled_counter;
    uint32_t moves_learned_counter;
    uint32_t nb_big_treasure_wins;
    uint32_t nb_recycled;
    uint32_t nb_gifts_sent;
    uint32_t pokemon_joined_flags[37];
    uint32_t pokemon_battled_flags[37];
    uint32_t moves_learned_flags[17];
    uint32_t items_acquired_flags[44];
    uint32_t special_challenge_flags;
    uint32_t sentry_duty_game_points[5];
    struct dungeon_floor_pair current_floor;
    uint16_t padding;
};
ASSERT_SIZE(struct adventure_log, 636);

// a 2d uint (32bit) vector
struct uvec2 {
    uint32_t x;
    uint32_t y;
};
ASSERT_SIZE(struct uvec2, 8);

// a 2d int (32bit) vector
struct vec2 {
    int32_t x;
    int32_t y;
};
ASSERT_SIZE(struct vec2, 8);

struct exclusive_item_stat_boost_entry {
    int8_t atk;
    int8_t sp_atk;
    int8_t def;
    int8_t sp_def;
};
ASSERT_SIZE(struct exclusive_item_stat_boost_entry, 4);

struct exclusive_item_effect_entry {
    struct exclusive_item_effect_id_8 effect_id;
    uint8_t foreign_idx; // Index into other tables
};
ASSERT_SIZE(struct exclusive_item_effect_entry, 2);

struct rankup_table_entry {
    undefined field_0x0;
    undefined field_0x1;
    undefined field_0x2;
    undefined field_0x3;
    int field_0x4;
    int field_0x8;
    int16_t field_0xc;
    undefined field_0xe;
    undefined field_0xf;
};
ASSERT_SIZE(struct rankup_table_entry, 16);

// Contains the data for a single mission
struct mission {
    struct mission_status_8 status; // 0x0
    struct mission_type_8 type;     // 0x1
    union mission_subtype subtype;  // 0x2
    undefined field_0x3;
    struct dungeon_id_8 dungeon_id; // 0x4
    uint8_t floor;                  // 0x5
    undefined field_0x6;            // Likely padding
    undefined field_0x7;            // Likely padding
    int field_0x8;                  // 0x8, changing it seems to affect the text of the mission
    undefined field_0xc;
    undefined field_0xd;
    struct monster_id_16 client; // 0xE
    struct monster_id_16 target; // 0x10
    int16_t field_0x12;
    int16_t field_0x14;
    struct mission_reward_type_8 reward_type; // 0x16
    undefined field_0x17;
    struct item_id_16 item_reward;                      // 0x18
    struct mission_restriction_type_8 restriction_type; // 0x1A
    undefined field_0x1b;
    union mission_restriction restriction; // 0x1C
    undefined field_0x1e;
    undefined field_0x1f;
};
ASSERT_SIZE(struct mission, 32);

struct mission_floors_forbidden {
    uint8_t field_0x0;
    uint8_t field_0x1;
};
ASSERT_SIZE(struct mission_floors_forbidden, 2);

// Unverified, ported from Irdkwia's notes
struct quiz_answer_points_entry {
    undefined field_0x0;
    undefined field_0x1;
    undefined field_0x2;
    undefined field_0x3;
    undefined field_0x4;
    undefined field_0x5;
    undefined field_0x6;
    undefined field_0x7;
    undefined field_0x8;
    undefined field_0x9;
    undefined field_0xa;
    undefined field_0xb;
    undefined field_0xc;
    undefined field_0xd;
    undefined field_0xe;
    undefined field_0xf;
};
ASSERT_SIZE(struct quiz_answer_points_entry, 16);

// Unverified, ported from Irdkwia's notes
struct portrait_data_entry {
    int16_t xpos;
    int16_t ypos;
    uint8_t portrait;
    uint8_t _padding;
};
ASSERT_SIZE(struct portrait_data_entry, 6);

// Unverified, ported from Irdkwia's notes
struct status_description {
    int16_t name_str_id;
    int16_t desc_str_id;
};
ASSERT_SIZE(struct status_description, 4);

// Unverified, ported from Irdkwia's notes
struct forbidden_forgot_move_entry {
    struct monster_id_16 monster_id;
    struct dungeon_id_16 origin_id;
    struct move_id_16 move_id;
};
ASSERT_SIZE(struct forbidden_forgot_move_entry, 6);

struct version_exclusive_monster {
    struct monster_id_16 id;
    bool in_eot; // In Explorers of Time
    bool in_eod; // In Explorers of Darkness
};
ASSERT_SIZE(struct version_exclusive_monster, 4);

// TODO: Add more data file structures, as convenient or needed, especially if the load address
// or pointers to the load address are known.

#endif
