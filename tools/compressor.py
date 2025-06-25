import math
from collections import Counter, defaultdict
from dataclasses import dataclass
from typing import Any, Generator

from intervaltree import Interval, IntervalTree

@dataclass(frozen=True)
class Repeat():
    value: Any
    repetitions: int

@dataclass(frozen=True)
class Match():
    id: int

def _get_subarray_indices(arr, min_len) -> Generator[tuple[int, int, int, int]]:
    def is_sequence_of_same(line):
        if len(line) == 0:
            return False

        first = line[0]

        # there technically should not be repetition or match markers at this stage
        # still, for clarity, lets make sure we don't consider such sequences

        return all(x == first and isinstance(x, int) for x in line)

    # Call is funny. rcall takes 3 CPU cycles which is 1.5 sbi / cbi instructions. We have no way but roundup to 4 cycles
    # -3 (outer_start)      -2 -1                  0 (inner_start)
    # set-level (x2)        nop(x1) + call (x3)    any new instruction
    num_ops_match_start = 3

    # ret takes 4 cycles (2 'instructions'), so we need something that keeps the level for 3 instructions:
    # -1 (inner_end - 1)    +0, +1       +2 (outer_start)
    # set-level (x2)        ret (x4)     any new instruction
    # adding to that, it's 1 instruction in match itself and 2 outside the match, so it's tricky...
    num_ops_before_match_end = 1
    num_ops_after_match_end = 2

    for inner_start in range(num_ops_match_start, len(arr)):
        outer_start = inner_start - num_ops_match_start

        # that +1 here is because python slice indices are non-inclusive?
        for inner_end in range(inner_start + min_len, len(arr) - num_ops_after_match_end + 1):
            outer_end = inner_end + num_ops_after_match_end

            if not is_sequence_of_same(arr[outer_start:inner_start]):
                continue

            if not is_sequence_of_same(arr[inner_end - num_ops_before_match_end:outer_end]):
                continue

            yield (outer_start, inner_start, inner_end, outer_end)

def _find_repeated_pixels(line: tuple, min_repetitions: int, preserve_trailing=False) -> Generator[tuple[Any, int]]:
    min_following = min_repetitions - 1
    current = line[0]
    following = 0

    def dump_values():
        if following >= min_following:
            yield (current, following)
        else:
            # Emit the value itself + following occurences
            for _ in range(following + 1):
                yield (current, 0)

    for x in range(1, len(line)):
        if line[x] == current:
            following += 1
        else:
            yield from dump_values()

            current = line[x]
            following = 0

    if not preserve_trailing and current == 0:
        # we don't really care about the reminder as it's the end of scanline. Just yield the last value
        yield (current, 0)
    else:
        yield from dump_values()

def _compress_run_length(lines: list[tuple], min_repetitions: int, preserve_trailing=False) -> Generator[tuple]:
    for line in lines:
        encoded_line = []
        for value, following in _find_repeated_pixels(line, min_repetitions, preserve_trailing):
            if following == 0:
                encoded_line.append(value)
            else:
                encoded_line.append(Repeat(value, following))

        yield tuple(encoded_line)

def _find_substitutions(lines, min_len) -> list[tuple[tuple, list[Interval]]]:
    substitutions: defaultdict[tuple, list[Interval]] = defaultdict(list)

    for y, line in enumerate(lines):
        for outer_start, inner_start, inner_end, outer_end in _get_subarray_indices(line, min_len):
            sequence = line[inner_start:inner_end]

            # bit of a hack here to use intervaltree as temp data storage. Data types has to be revisited here...
            substitutions[sequence].append(Interval(outer_start, outer_end, (y, inner_start, inner_end)))

    # sort by impact - longest sequences and most possible substitutions
    def sorting_key(pair):
        sequence, substitutions = pair
        match_marker_size = 1

        before = len(sequence) * len(substitutions)
        after = len(sequence) + len(substitutions) * match_marker_size

        return before - after

    def entropy(seq):
        counts = Counter(seq)
        total = len(seq)
        return -sum((c/total) * math.log2(c/total) for c in counts.values())

    def sorting_key_entropy(pair):
        sequence, substitutions = pair
        match_marker_size = 1
        before = len(sequence) * len(substitutions)
        after = len(sequence) + len(substitutions) * match_marker_size
        gain = before - after

        # Weight by entropy to penalize low-information sequences
        return gain * entropy(sequence)

    # we need at least 2 occurences of each sequence. sort them as well
    return sorted(
        (x for x in substitutions.items() if len(x[1]) >= 2), key=sorting_key_entropy, reverse=True
    )

def _compress_substitutions(lines: list[tuple], min_substitution_len: int) -> tuple[list[tuple], dict]:
    output = []
    trees: defaultdict[int, IntervalTree] = defaultdict(IntervalTree)
    dictionary: dict[tuple, Match] = {}

    for sequence, substitutions in _find_substitutions(lines, min_substitution_len):
        for substitution in substitutions:
            y, _, _= substitution.data

            if not trees[y].overlap(substitution):
                trees[y].add(substitution)

    match_id_counter = 0

    for y, reference in enumerate(lines):
        # work right to left, so we don't need index adjustment
        substitutions: list[Interval] = sorted(trees[y], key=lambda s: s.begin, reverse=True)

        line = list(reference)

        for i, substitution in enumerate(substitutions):
            # that's super wonky :)
            _, inner_start, inner_end = substitution.data
            outer_start, outer_end = substitution.begin, substitution.end

            sequence = tuple(reference[inner_start:inner_end])

            if sequence not in dictionary:
                match_marker = Match(match_id_counter)
                dictionary[sequence] = match_marker
                match_id_counter += 1
            else:
                match_marker = dictionary[sequence]

            line[outer_start + 1:outer_end] = [ match_marker ]

        output.append(tuple(line))

    return (output, dict((v,k) for k,v in dictionary.items()))

def compress(lines: list[tuple], min_substitution_len: int, min_rle_repetitions: int, preserve_trailing=True) -> tuple[list[tuple], dict]:
    mode = 2

    if mode == 0:
        return list(_compress_run_length(lines, min_rle_repetitions, preserve_trailing)), {}
    elif mode == 1:
        return _compress_substitutions(lines, min_substitution_len)
    elif mode == 2:
        rle = list(_compress_run_length(lines, min_rle_repetitions, preserve_trailing))
        return _compress_substitutions(rle, min_substitution_len)
    elif mode == 3:
        subst, dictionary = _compress_substitutions(lines, min_substitution_len)

        output = list(_compress_run_length(subst, min_rle_repetitions, preserve_trailing))

        for name, sequence in dictionary.items():
            dictionary[name] = list(_compress_run_length([sequence], min_rle_repetitions, preserve_trailing=True))[0]

        return output, dictionary
    elif mode == 4:
        return lines, {}

    return [], {}

def decompress(lines: list[tuple], dictionary: dict[Match, tuple]) -> Generator[tuple]:
    for line in lines:
        output_line = []

        for x in range(len(line)):
            item = line[x]
            if isinstance(item, int):
                output_line += [item]
            elif isinstance(item, Match):
                sequence = dictionary[item]

                # here we simulate gaps needed to go in and out of the match

                # call (set + call op)
                current_level = line[x-1]
                # if not isinstance(current_level, int):
                #     raise Exception()

                output_line += [ current_level, current_level ]
                output_line += sequence

                sequence_end_level = sequence[-1]

                # ret (2 ops)
                output_line += [ sequence_end_level, sequence_end_level ]

            elif isinstance(item, Repeat):
                # don't unpack it now, we'll do another pass at the end (matches can also contain repetitions)
                output_line += [item]

        for x, item in enumerate(output_line):
            if isinstance(item, Repeat):
                output_line[x:x+1] = [ item.value ] * (item.repetitions + 1)

        yield tuple(output_line)
