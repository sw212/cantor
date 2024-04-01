#ifndef HASH_H
#define HASH_H

struct Hash_Scope
{
    u64 v[1];
};

struct Hash_Node
{
    Arena* arena;

    Hash_Node* hash_next;
    Hash_Node* hash_prev;

    u128 hash;
    Str8 data;
    u64  last_accessed_time_us;
};

struct Hash_Slot
{
    Hash_Node* first;
    Hash_Node* last;
};

struct Hash_State
{
    Arena* arena;

    Hash_Slot* hash_table;
    u64 table_size;

    Sys_ShardTable* stripe_table;
};

function void Hash_Initialise();

function Hash_Scope* Hash_ScopeOpen();
function void        Hash_ScopeClose(Hash_Scope* scope);

function u128 Hash_FromString(Str8 string);

function u128 Hash_SubmitData(Arena** arena, Str8 data);
function void Hash_SubmitStaticData(Str8 data, u128 hash);
function Str8 Hash_DataFromHash(Hash_Scope* scope, u128 hash);

#endif