#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <math.h>

struct stack_entry {
        int next_row;
};

struct queen {
        int x, y;
        int sequence;
};

struct solution_state {
        struct queen *queens;
        float total_nearest_distance;
};

static void
get_diagonals(int grid_size,
              int x, int y,
              int *forward_diagonal,
              int *back_diagonal)
{
        *forward_diagonal = x + y;
        *back_diagonal = grid_size - 1 - x + y;
}

static bool
check_bit(const uint8_t *mask,
          int index)
{
        return mask[index / 8] & (1 << (index % 8));
}

static void
set_bit(uint8_t *mask, int index)
{
        mask[index / 8] |= 1 << (index % 8);
}

static void
reset_bit(uint8_t *mask, int index)
{
        mask[index / 8] &= ~(1 << (index % 8));
}

static int
xdigit(int val)
{
        return val < 10 ? val + '0' : val - 10 + 'a';
}

static int
get_queen_distance2(const struct queen *q)
{
        return q->x * q->x + q->y * q->y;
}

static int
compare_queen_center_distance(const void *a,
                              const void *b)
{
        const struct queen * const *qa = a;
        const struct queen * const *qb = b;
        int da = get_queen_distance2(*qa);
        int db = get_queen_distance2(*qb);

        return da - db;
}

static void
print_solution(int grid_size,
               const struct queen *queens_in)
{
        int n_values = (grid_size + 3) / 4;
        uint32_t values[n_values];
        struct queen queens[grid_size];
        struct queen *sorted_queens[grid_size];
        int x, y, i;

        memset(values, 0, n_values * sizeof values[0]);

        for (x = 0; x < grid_size; x++) {
                queens[x].x = x - grid_size / 2;
                queens[x].y = queens_in[x].y - grid_size / 2;
                sorted_queens[x] = queens + x;
        }

        qsort(sorted_queens,
              grid_size,
              sizeof sorted_queens[0],
              compare_queen_center_distance);

        for (x = 0; x < grid_size; x++) {
                sorted_queens[x]->sequence = x;
                queens[x].x += grid_size / 2;
                queens[x].y += grid_size / 2;
        }

        fputs(" * Sample positions:\n"
              " *  ",
              stdout);
        for (x = 0; x < grid_size; x++)
                printf(" %c", xdigit(x * 16 / grid_size));
        fputc('\n', stdout);

        for (y = 0; y < grid_size; y++) {
                printf(" * %c", xdigit(y * 16 / grid_size));
                for (x = 0; x < grid_size; x++) {
                        if (queens[x].y == y) {
                                for (i = 0; i < x; i++)
                                        fputs("  ", stdout);
                                printf(" %c", xdigit(queens[x].sequence));
                                break;
                        }
                }
                fputc('\n', stdout);
        }

        printf(" */\n"
               "static const uint32_t\n"
               "brw_multisample_positions_%ix[] = {",
               grid_size);

        for (x = 0; x < grid_size; x++) {
                values[x / 4] |= ((sorted_queens[x]->x * 16 / grid_size)
                                  << (x % 4 * 8 + 4));
                values[x / 4] |= ((sorted_queens[x]->y * 16 / grid_size)
                                  << (x % 4 * 8));
        }

        for (x = 0; x < n_values; x++) {
                if (x > 0)
                        fputc(',', stdout);
                fputc(' ', stdout);
                printf("0x%08" PRIx32, values[x]);
        }
        printf(" };\n");
}

static void
handle_solution(int grid_size,
                const struct stack_entry *stack,
                struct solution_state *solution_state)
{
        struct queen queens[grid_size];
        int nearest_distance2;
        int distance2;
        float total_nearest_distance = 0;
        int dx, dy;
        int i, j;

        for (i = 0; i < grid_size; i++) {
                queens[i].x = i;
                queens[i].y = stack[i].next_row - 1;
        }

        for (i = 0; i < grid_size; i++) {
                nearest_distance2 = INT_MAX;

                for (j = 0; j < grid_size; j++) {
                        if (i == j)
                                continue;

                        dx = queens[i].x - queens[j].x;
                        dy = queens[i].y - queens[j].y;

                        distance2 = dx * dx + dy * dy;

                        if (distance2 < nearest_distance2)
                                nearest_distance2 = distance2;
                }

                total_nearest_distance += sqrtf(nearest_distance2);
        }

        if (total_nearest_distance > solution_state->total_nearest_distance) {
                solution_state->total_nearest_distance =
                        total_nearest_distance;
                memcpy(solution_state->queens, queens, sizeof queens);
        }
}

static void
search(int grid_size)
{
        struct stack_entry stack[grid_size];
        uint8_t row_mask[(grid_size + 7) / 8];
        int n_diagnols = grid_size * 2 - 1;
        uint8_t forward_diagonal_mask[(n_diagnols + 7) / 8];
        uint8_t back_diagonal_mask[(n_diagnols + 7) / 8];
        int forward_diagonal;
        int back_diagonal;
        int stack_size = 1;
        struct solution_state solution_state;
        int x, y;
        int max_row;

        solution_state.total_nearest_distance = -FLT_MAX;
        solution_state.queens = alloca(sizeof *solution_state.queens *
                                       grid_size);

        stack[0].next_row = 0;

        memset(row_mask, 0, sizeof row_mask);
        memset(forward_diagonal_mask, 0, sizeof forward_diagonal_mask);
        memset(back_diagonal_mask, 0, sizeof back_diagonal_mask);

        while (true) {
                x = stack_size - 1;
                y = stack[x].next_row;

                if (x == grid_size / 2)
                        max_row = grid_size / 2 + 1;
                else
                        max_row = grid_size;

                if (y < max_row) {
                        stack[stack_size - 1].next_row++;

                        get_diagonals(grid_size,
                                      x, y,
                                      &forward_diagonal, &back_diagonal);

                        if (!check_bit(row_mask, y) &&
                            !check_bit(forward_diagonal_mask,
                                       forward_diagonal) &&
                            !check_bit(back_diagonal_mask,
                                       back_diagonal)) {

                                if (stack_size >= grid_size) {
                                        handle_solution(stack_size,
                                                        stack,
                                                        &solution_state);
                                } else {
                                        set_bit(row_mask, y);
                                        set_bit(forward_diagonal_mask,
                                                forward_diagonal);
                                        set_bit(back_diagonal_mask,
                                                back_diagonal);

                                        if (stack_size == grid_size / 2)
                                                stack[stack_size].next_row =
                                                        grid_size / 2;
                                        else
                                                stack[stack_size].next_row = 0;
                                        stack_size++;
                                }
                        }
                } else {
                        /* Backtrack */
                        if (stack_size <= 0)
                                break;

                        stack_size--;

                        x = stack_size - 1;
                        y = stack[x].next_row - 1;
                        get_diagonals(grid_size,
                                      x, y,
                                      &forward_diagonal, &back_diagonal);

                        reset_bit(row_mask, y);
                        reset_bit(forward_diagonal_mask, forward_diagonal);
                        reset_bit(back_diagonal_mask, back_diagonal);
                }
        }

        print_solution(grid_size, solution_state.queens);
}

int
main(int argc, char **argv)
{
        int sample_count = 8;

        if (argc > 1) {
                sample_count = atoi(argv[1]);
                if (sample_count <= 0) {
                        fprintf(stderr,
                                "usage: queens [sample_count]\n");
                        return 1;
                }
        }

        search(sample_count);

        return 0;
}
