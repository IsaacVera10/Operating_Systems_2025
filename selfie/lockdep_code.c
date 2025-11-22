/*
 * LOCKDEP: Deadlock Avoidance System for Selfie OS
 * 
 * Este código debe insertarse en selfie.c ANTES de la sección MICROKERNEL
 * (alrededor de la línea 2450)
 * 
 * IMPORTANTE: Actualizar CONTEXTENTRIES de 35 a 37
 */

// *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~
// -----------------------------------------------------------------
// ----------------------    L O C K D E P    ----------------------
// -----------------------------------------------------------------
// *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~ *~*~

// -----------------------------------------------------------------
// --------------------- LOCKDEP DATA STRUCTURES -------------------
// -----------------------------------------------------------------

void init_lockdep();
void reset_lockdep();

uint64_t lockdep_lock_acquire(uint64_t *context, uint64_t lock_class);
void lockdep_lock_release(uint64_t *context, uint64_t lock_class);

void lockdep_semaphore_acquire(uint64_t *context, uint64_t sem_class);
void lockdep_semaphore_release(uint64_t *context, uint64_t sem_class);

void add_held_lock(uint64_t *context, uint64_t lock_class);
void remove_held_lock(uint64_t *context, uint64_t lock_class);
uint64_t is_lock_held(uint64_t *context, uint64_t lock_class);

uint64_t dependency_exists(uint64_t from, uint64_t to);
void add_dependency(uint64_t from, uint64_t to);

uint64_t would_create_cycle(uint64_t from, uint64_t to);
uint64_t has_cycle_dfs(uint64_t from, uint64_t to, uint64_t *visited, uint64_t depth);

void print_deadlock_warning(uint64_t *context, uint64_t from_class, uint64_t to_class);
void print_held_locks(uint64_t *context);
void print_dependency_chain(uint64_t from, uint64_t to);

// ------------------------ GLOBAL CONSTANTS -----------------------

uint64_t MAX_LOCKDEP_DEPENDENCIES = 512;
uint64_t MAX_LOCKDEP_HELD_LOCKS = 16;

uint64_t DEPENDENCY_ENTRIES = 3;
uint64_t HELD_LOCK_ENTRIES = 2;

// ------------------------ GLOBAL VARIABLES -----------------------

uint64_t *lockdep_dependency_graph = (uint64_t *)0;
uint64_t lockdep_total_dependencies = 0;
uint64_t lockdep_enabled = 1;

uint64_t *lockdep_visited_buffer = (uint64_t *)0;

// -----------------------------------------------------------------
// ------------------- DEPENDENCY STRUCTURE ------------------------
// -----------------------------------------------------------------

// dependency_struct (lock dependency edge)
// +---+------------------+
// | 0 | from             | source lock class
// | 1 | to               | destination lock class
// | 2 | next             | next dependency in list
// +---+------------------+

uint64_t *allocate_dependency()
{
    return smalloc(DEPENDENCY_ENTRIES * sizeof(uint64_t));
}

uint64_t get_dependency_from(uint64_t *dep)
{
    return *dep;
}

uint64_t get_dependency_to(uint64_t *dep)
{
    return *(dep + 1);
}

uint64_t *get_dependency_next(uint64_t *dep)
{
    return (uint64_t *)*(dep + 2);
}

void set_dependency_from(uint64_t *dep, uint64_t from)
{
    *dep = from;
}

void set_dependency_to(uint64_t *dep, uint64_t to)
{
    *(dep + 1) = to;
}

void set_dependency_next(uint64_t *dep, uint64_t *next)
{
    *(dep + 2) = (uint64_t)next;
}

// -----------------------------------------------------------------
// -------------------- HELD LOCK STRUCTURE ------------------------
// -----------------------------------------------------------------

// held_lock_struct (lock held by a context)
// +---+------------------+
// | 0 | lock_class       | lock identifier
// | 1 | next             | next held lock in list
// +---+------------------+

uint64_t *allocate_held_lock()
{
    return smalloc(HELD_LOCK_ENTRIES * sizeof(uint64_t));
}

uint64_t get_held_lock_class(uint64_t *held)
{
    return *held;
}

uint64_t *get_held_lock_next(uint64_t *held)
{
    return (uint64_t *)*(held + 1);
}

void set_held_lock_class(uint64_t *held, uint64_t lock_class)
{
    *held = lock_class;
}

void set_held_lock_next(uint64_t *held, uint64_t *next)
{
    *(held + 1) = (uint64_t)next;
}

// -----------------------------------------------------------------
// -------------------- CONTEXT EXTENSIONS -------------------------
// -----------------------------------------------------------------

// Context extensions for lockdep
// | 35 | held_locks_head  | pointer to first held lock
// | 36 | held_locks_count | number of locks held

uint64_t held_locks_head(uint64_t *context)
{
    return (uint64_t)(context + 35);
}

uint64_t held_locks_count(uint64_t *context)
{
    return (uint64_t)(context + 36);
}

uint64_t *get_held_locks_head(uint64_t *context)
{
    return (uint64_t *)*(context + 35);
}

uint64_t get_held_locks_count(uint64_t *context)
{
    return *(context + 36);
}

void set_held_locks_head(uint64_t *context, uint64_t *head)
{
    *(context + 35) = (uint64_t)head;
}

void set_held_locks_count(uint64_t *context, uint64_t count)
{
    *(context + 36) = count;
}

// -----------------------------------------------------------------
// ------------------- HELD LOCKS MANAGEMENT -----------------------
// -----------------------------------------------------------------

void add_held_lock(uint64_t *context, uint64_t lock_class)
{
    uint64_t *held_lock;
    uint64_t *head;
    uint64_t count;

    count = get_held_locks_count(context);

    if (count >= MAX_LOCKDEP_HELD_LOCKS)
    {
        printf("LOCKDEP WARNING: Max held locks (%lu) exceeded by context %lu\n",
               MAX_LOCKDEP_HELD_LOCKS, get_id_context(context));
        return;
    }

    held_lock = allocate_held_lock();
    set_held_lock_class(held_lock, lock_class);

    head = get_held_locks_head(context);
    set_held_lock_next(held_lock, head);

    set_held_locks_head(context, held_lock);
    set_held_locks_count(context, count + 1);
}

void remove_held_lock(uint64_t *context, uint64_t lock_class)
{
    uint64_t *prev;
    uint64_t *current;
    uint64_t *next;
    uint64_t count;

    prev = (uint64_t *)0;
    current = get_held_locks_head(context);

    while (current != (uint64_t *)0)
    {
        if (get_held_lock_class(current) == lock_class)
        {
            if (prev == (uint64_t *)0)
            {
                next = get_held_lock_next(current);
                set_held_locks_head(context, next);
            }
            else
            {
                next = get_held_lock_next(current);
                set_held_lock_next(prev, next);
            }

            count = get_held_locks_count(context);
            set_held_locks_count(context, count - 1);
            return;
        }

        prev = current;
        current = get_held_lock_next(current);
    }
}

uint64_t is_lock_held(uint64_t *context, uint64_t lock_class)
{
    uint64_t *current;

    current = get_held_locks_head(context);

    while (current != (uint64_t *)0)
    {
        if (get_held_lock_class(current) == lock_class)
        {
            return 1;
        }
        current = get_held_lock_next(current);
    }

    return 0;
}

// -----------------------------------------------------------------
// ---------------- DEPENDENCY GRAPH MANAGEMENT --------------------
// -----------------------------------------------------------------

uint64_t dependency_exists(uint64_t from, uint64_t to)
{
    uint64_t *dep;

    dep = lockdep_dependency_graph;

    while (dep != (uint64_t *)0)
    {
        if (get_dependency_from(dep) == from)
        {
            if (get_dependency_to(dep) == to)
            {
                return 1;
            }
        }
        dep = get_dependency_next(dep);
    }

    return 0;
}

void add_dependency(uint64_t from, uint64_t to)
{
    uint64_t *new_dep;

    if (lockdep_total_dependencies >= MAX_LOCKDEP_DEPENDENCIES)
    {
        printf("LOCKDEP WARNING: Max dependencies (%lu) reached\n",
               MAX_LOCKDEP_DEPENDENCIES);
        return;
    }

    new_dep = allocate_dependency();
    set_dependency_from(new_dep, from);
    set_dependency_to(new_dep, to);
    set_dependency_next(new_dep, lockdep_dependency_graph);

    lockdep_dependency_graph = new_dep;
    lockdep_total_dependencies = lockdep_total_dependencies + 1;
}

// -----------------------------------------------------------------
// ------------------- CYCLE DETECTION (DFS) -----------------------
// -----------------------------------------------------------------

void lockdep_init_visited(uint64_t *visited, uint64_t size)
{
    uint64_t i;

    i = 0;
    while (i < size)
    {
        *(visited + i) = 0;
        i = i + 1;
    }
}

uint64_t lockdep_is_visited(uint64_t *visited, uint64_t depth, uint64_t node)
{
    uint64_t i;

    i = 0;
    while (i < depth)
    {
        if (*(visited + i) == node)
        {
            return 1;
        }
        i = i + 1;
    }

    return 0;
}

uint64_t has_cycle_dfs(uint64_t from, uint64_t to,
                        uint64_t *visited, uint64_t depth)
{
    uint64_t *dep;
    uint64_t dep_from;
    uint64_t dep_to;
    uint64_t already_visited;

    if (depth > MAX_LOCKDEP_HELD_LOCKS)
    {
        return 1;
    }

    if (from == to)
    {
        return 1;
    }

    *(visited + depth) = from;

    dep = lockdep_dependency_graph;

    while (dep != (uint64_t *)0)
    {
        dep_from = get_dependency_from(dep);
        dep_to = get_dependency_to(dep);

        if (dep_from == from)
        {
            already_visited = lockdep_is_visited(visited, depth, dep_to);

            if (already_visited == 0)
            {
                if (has_cycle_dfs(dep_to, to, visited, depth + 1))
                {
                    return 1;
                }
            }
        }

        dep = get_dependency_next(dep);
    }

    return 0;
}

uint64_t would_create_cycle(uint64_t from, uint64_t to)
{
    uint64_t *visited;
    uint64_t result;

    visited = lockdep_visited_buffer;
    lockdep_init_visited(visited, MAX_LOCKDEP_HELD_LOCKS);

    result = has_cycle_dfs(to, from, visited, 0);

    return result;
}

// -----------------------------------------------------------------
// -------------------- DEADLOCK WARNINGS --------------------------
// -----------------------------------------------------------------

void print_dependency_chain(uint64_t from, uint64_t to)
{
    uint64_t *dep;
    uint64_t depth;

    printf("  Dependency chain:\n");

    dep = lockdep_dependency_graph;
    depth = 0;

    while (dep != (uint64_t *)0)
    {
        if (depth < 10)
        {
            printf("    [%lu] 0x%lX -> 0x%lX\n",
                   depth,
                   get_dependency_from(dep),
                   get_dependency_to(dep));
            depth = depth + 1;
        }
        dep = get_dependency_next(dep);
    }
}

void print_held_locks(uint64_t *context)
{
    uint64_t *held;
    uint64_t count;

    printf("  Currently held locks by context %lu:\n",
           get_id_context(context));

    held = get_held_locks_head(context);
    count = 0;

    while (held != (uint64_t *)0)
    {
        printf("    [%lu] lock_class = 0x%lX\n",
               count,
               get_held_lock_class(held));
        held = get_held_lock_next(held);
        count = count + 1;
    }
}

void print_deadlock_warning(uint64_t *context,
                             uint64_t from_class,
                             uint64_t to_class)
{
    printf("\n");
    printf("======================================================\n");
    printf("LOCKDEP: DEADLOCK DETECTED!\n");
    printf("======================================================\n");
    printf("Context %lu attempting to acquire lock 0x%lX\n",
           get_id_context(context), to_class);
    printf("while already holding lock 0x%lX\n", from_class);
    printf("\n");
    printf("This would create a circular dependency:\n");
    printf("  0x%lX -> 0x%lX (new)\n", from_class, to_class);
    printf("  0x%lX -> ... -> 0x%lX (existing)\n", to_class, from_class);
    printf("\n");

    print_held_locks(context);
    printf("\n");
    print_dependency_chain(from_class, to_class);

    printf("\n");
    printf("*** LOCK ACQUISITION DENIED ***\n");
    printf("======================================================\n");
    printf("\n");
}

// -----------------------------------------------------------------
// ------------------- MAIN LOCKDEP FUNCTIONS ----------------------
// -----------------------------------------------------------------

uint64_t lockdep_lock_acquire(uint64_t *context, uint64_t lock_class)
{
    uint64_t *held;
    uint64_t from_class;

    if (lockdep_enabled == 0)
    {
        return 1;
    }

    held = get_held_locks_head(context);

    while (held != (uint64_t *)0)
    {
        from_class = get_held_lock_class(held);

        if (dependency_exists(from_class, lock_class) == 0)
        {
            if (would_create_cycle(from_class, lock_class))
            {
                print_deadlock_warning(context, from_class, lock_class);
                return 0;
            }

            add_dependency(from_class, lock_class);
        }

        held = get_held_lock_next(held);
    }

    add_held_lock(context, lock_class);
    return 1;
}

void lockdep_lock_release(uint64_t *context, uint64_t lock_class)
{
    if (lockdep_enabled == 0)
    {
        return;
    }

    remove_held_lock(context, lock_class);
}

void lockdep_semaphore_acquire(uint64_t *context, uint64_t sem_class)
{
    lockdep_lock_acquire(context, sem_class);
}

void lockdep_semaphore_release(uint64_t *context, uint64_t sem_class)
{
    lockdep_lock_release(context, sem_class);
}

// -----------------------------------------------------------------
// --------------------- INITIALIZATION ----------------------------
// -----------------------------------------------------------------

void init_lockdep()
{
    lockdep_dependency_graph = (uint64_t *)0;
    lockdep_total_dependencies = 0;
    lockdep_enabled = 1;

    lockdep_visited_buffer = smalloc(MAX_LOCKDEP_HELD_LOCKS * sizeof(uint64_t));
    lockdep_init_visited(lockdep_visited_buffer, MAX_LOCKDEP_HELD_LOCKS);

    printf("LOCKDEP: Initialized (max_deps=%lu, max_held=%lu)\n",
           MAX_LOCKDEP_DEPENDENCIES, MAX_LOCKDEP_HELD_LOCKS);
}

void reset_lockdep()
{
    lockdep_dependency_graph = (uint64_t *)0;
    lockdep_total_dependencies = 0;
    lockdep_enabled = 1;
}
