Hash_State* hash_state = 0;

function void
Hash_Initialise()
{
    if (IsCurrentThreadMain() && !hash_state)
    {
        Arena* arena = ArenaAlloc(Megabytes(16));
        hash_state = PushArray(arena, Hash_State, 1);
        hash_state->arena = arena;
        hash_state->table_size = 1024;
        hash_state->hash_table = PushArray(arena, Hash_Slot, hash_state->table_size);
        hash_state->stripe_table = Sys_ShardTableAlloc(arena, hash_state->table_size / 8);
    }
}

function Hash_Scope* 
Hash_ScopeOpen()
{
    Hash_Scope* scope = (Hash_Scope*)1;
    return scope;
}

function void
Hash_ScopeClose(Hash_Scope* scope)
{
    //
}

function u128
Hash_FromString(Str8 string)
{
    u128 result = {};

    meow_u128 hash = MeowHash(MeowDefaultSeed, string.size, string.str);
    MemoryCopy(&result, &hash, sizeof(meow_u128));

    return result;
}

function u128
Hash_SubmitData(Arena** arena, Str8 data)
{
    u128 hash = Hash_FromString(data);

    u64   slot_idx = hash.v[1] % hash_state->table_size;
    u64 stripe_idx = slot_idx  % hash_state->stripe_table->count;

    Hash_Slot*    slot = &hash_state->hash_table[slot_idx];
    Sys_Shard* stripe = &hash_state->stripe_table->stripes[stripe_idx];

    b32 is_new = 0;

    Sys_Mutex(stripe->mutex)
    {
        Hash_Node* existing_node = 0;

        for(Hash_Node* n = slot->first; n != 0; n = n->hash_next)
        {
            if(U128_Match(n->hash, hash))
            {
                existing_node = n;
                break;
            }
        }

        if(!existing_node)
        {
            Hash_Node* node = PushArray(stripe->arena, Hash_Node, 1);
            node->arena = *arena;
            node->hash = hash;
            node->data = data;
            Sys_Protect(data.str, data.size, Sys_Permission_R);
            DLLPushBack_NPZ(slot->first, slot->last, node, hash_next, hash_prev, CheckNull, SetNull);
            is_new = 1;
        }
    }

    // if new -> signal
    if(is_new)
    {
        Sys_CVSignalAll(stripe->cv);
    }

    // if not new -> release arena
    if(is_new == 0)
    {
        Arena* a = *arena;
        ArenaRelease(a);
    }

    *arena = 0;

    return hash;
}

function void
Hash_SubmitStaticData(Str8 data, u128 hash)
{
    u64   slot_idx = hash.v[1] % hash_state->table_size;
    u64 stripe_idx = slot_idx  % hash_state->stripe_table->count;

    Hash_Slot*    slot = &hash_state->hash_table[slot_idx];
    Sys_Shard* stripe = &hash_state->stripe_table->stripes[stripe_idx];

    b32 is_new = 0;

    Sys_Mutex(stripe->mutex)
    {
        Hash_Node* existing_node = 0;

        for(Hash_Node* n = slot->first; n != 0; n = n->hash_next)
        {
            if(U128_Match(n->hash, hash))
            {
                existing_node = n;
                break;
            }
        }

        if(!existing_node)
        {
            Hash_Node* node = PushArray(stripe->arena, Hash_Node, 1);
            node->arena = 0;
            node->hash = hash;
            node->data = data;
            DLLPushBack_NPZ(slot->first, slot->last, node, hash_next, hash_prev, CheckNull, SetNull);
            is_new = 1;
        }
    }

    // if new -> signal
    if(is_new)
    {
        Sys_CVSignalAll(stripe->cv);
    }
}

function Str8
Hash_DataFromHash(Hash_Scope* scope, u128 hash)
{
    Str8 result = {};
    
    u64   slot_idx = hash.v[1] % hash_state->table_size;
    u64 stripe_idx = slot_idx % hash_state->stripe_table->count;

    Sys_Shard* stripe = &hash_state->stripe_table->stripes[stripe_idx];
    Hash_Slot*    slot = &hash_state->hash_table[slot_idx];

    Sys_Mutex(stripe->mutex)
    {
        Hash_Node* existing_node = 0;

        for(Hash_Node* n = slot->first; n != 0; n = n->hash_next)
        {
            if(U128_Match(n->hash, hash))
            {
                existing_node = n;
                break;
            }
        }

        if(existing_node != 0)
        {
            result = existing_node->data;
            existing_node->last_accessed_time_us = Sys_TimeUS();
        }
    }

    return result; 
}