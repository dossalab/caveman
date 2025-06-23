import logging
import unittest

from compressor import compress, decompress, _get_subarray_indices, _compress_run_length, Repeat, Match, _compress_substitutions

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class TestCompressor(unittest.TestCase):
    def deep_size(self, arrays):
        return sum(len(a) for a in arrays)
        
    def test_compress_decompress(self):
        dataset = [
            (0, 0, 0, 1, 2, 3, 4, 0, 0, 0, 0),
            (0, 0, 0, 1, 2, 3, 4, 0, 0, 0, 0),
            (0, 0, 0, 1, 2, 3, 4, 0, 0, 0, 0)
        ]

        compressed, dictionary = compress(dataset, 5, 5)
        decompressed = list(decompress(compressed, dictionary))

        original_size = self.deep_size(dataset)
        compressed_size = self.deep_size(compressed) + self.deep_size(dictionary.values())

        logger.info('original size: %d items, %d byte(s)', original_size, original_size * 2)
        logger.info('compressed size: %d items, %d byte(s)', compressed_size, compressed_size * 2)
        logger.info('dictionary: %d items', len(dictionary))
        logger.info("ratio: %.2f", original_size / compressed_size)

        self.assertEqual(dataset, decompressed)

    def test_rle(self):
        dataset = [
            (
                True,
                5,
                ( 1, 2, 3, 4, 5, 6, 7 ),
                ( 1, 2, 3, 4, 5, 6, 7 )
            ),
            (
                True,
                5,
                ( 1, 2, 2, 2 ),
                ( 1, 2, 2, 2 )
            ),
            (
                True,
                5,
                ( 0, 0, 0, 0 ),
                ( 0, 0, 0, 0 )
            ),
            (
                True,
                5,
                ( 0, 0, 0, 0, 0 ),
                ( Repeat(0, 4), )
            ),
            (
                False,
                100,
                ( 0, 0, 0, 0, 0, 0 ),
                ( 0, )
            )
        ]

        for i, (preserve_trailing, min_len, data, expected) in enumerate(dataset):
            with self.subTest(i=i, input=data):
                results = list(_compress_run_length([data], min_len, preserve_trailing))
                self.assertEqual(results, [expected])

    def test_substitutions(self):
        dataset = [
            (
                5,
                [
                    (0, 0, 0, 1, 2, 3, 4, 5, 5, 5),
                    (0, 0, 0, 1, 2, 3, 4, 5, 5, 5),
                ],
                [
                    (0, Match(id=0)),
                    (0, Match(id=0))
                ],
                { Match(id=0): (1, 2, 3, 4, 5)}
            ),
            (
                5,
                [
                    (0, 0, 0, 1, 2, 3, 4, 5, 5, 5),
                    (1, 1, 1, 1, 2, 3, 4, 5, 5, 5),
                ],
                [
                    (0, Match(id=0)),
                    (1, Match(id=0)),
                ],
                { Match(id=0): (1, 2, 3, 4, 5)}
            ),
            (
                6,
                [
                    (0, 0, 1, 2, 3, 4, 5, 5, 5),
                    (0, 0, 1, 2, 3, 4, 5, 5, 5),
                ],
                [
                    (0, 0, 1, 2, 3, 4, 5, 5, 5),
                    (0, 0, 1, 2, 3, 4, 5, 5, 5),
                ],
                {}
            ),
            (
                5,
                [
                    (0, 0, 1, 2, 3, 4, 5, 0, 0),
                    (0, 0, 1, 2, 3, 4, 5, 0, 0),
                ],
                [
                    (0, 0, 1, 2, 3, 4, 5, 0, 0),
                    (0, 0, 1, 2, 3, 4, 5, 0, 0),
                ],
                {}
            ),
            (
                5,
                [
                    (0, 1, 2, 3, 4, 5, 5, 5),
                    (0, 1, 2, 3, 4, 5, 5, 5),
                ],
                [
                    (0, 1, 2, 3, 4, 5, 5, 5),
                    (0, 1, 2, 3, 4, 5, 5, 5),
                ],
                {}
            ),
            (
                5,
                [
                    (0, 0, 0, 1, 2, 3, 4, 5, 5, 5, 0, 0, 0, 1, 2, 9, 4, 5, 5, 5),
                    (0, 0, 0, 1, 2, 9, 4, 5, 5, 5, 0, 0, 0, 1, 2, 3, 4, 5, 5, 5),
                ],
                [
                    (0, Match(id=1), 0, Match(id=0)),
                    (0, Match(id=0), 0, Match(id=1))
                ],
                {
                    Match(id=0): (1, 2, 9, 4, 5),
                    Match(id=1): (1, 2, 3, 4, 5)
                }
            ),
            (
                5,
                [
                    #    .  .  .  .  .  .  .  .  .
                    # S  S  S  M  M  M  M  M  E  E  x  x  x  x  x  x
                    ( 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0),
                    ( 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
                ],
                [
                    (1, Match(id=0), 1, 1, 1, 0, 0, 0),
                    (1, Match(id=0), 1, 1, 1, 1, 1, 0)
                ],
                {
                    Match(id=0): (1, 1, 1, 1, 1),
                }
            ),
            (
                5,
                [
                    #    .  .  .  .  .  .  .  .  .     .  .  .  .  .  .  .  .  .
                    # S  S  S  M  M  M  M  M  E  E  S  S  S  M  M  M  M  M  E  E
                    ( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
                    ( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
                ],
                [
                    (0, Match(id=0), 0, Match(id=0)),
                    (0, Match(id=0), 0, Match(id=0))
                ],
                {
                    Match(id=0): (0, 0, 0, 0, 0)
                }
            )
        ]

        for i, (min_len, data, expected, expected_dictionary) in enumerate(dataset):
            with self.subTest(i=i, input=data):
                results, dictionary = _compress_substitutions(data, min_len)
                self.assertEqual(results, expected)
                self.assertEqual(dictionary, expected_dictionary)

    def test_subarray_indices(self):
        dataset = [
            (
                2,

                # S  S  S  M  M  M  M  E  E
                [ 1, 1, 1, 9, 8, 2, 2, 2, 2 ],
                [(0, 3, 6, 8), (0, 3, 7, 9)],
            ),
            (
                2,

                [1, 1, 1, 2, 2, 3, 2, 1, 2],
                []
            ),
            (
                6,

                [ 1, 1, 1, 2, 2, 3, 2, 2, 2 ],
                []
            )
        ]

        for i, (min_len, data, expected) in enumerate(dataset):
            with self.subTest(i=i, input=data):
                results = list(_get_subarray_indices(data, min_len))
                self.assertEqual(results, expected)

if __name__ == '__main__':
    unittest.main()
