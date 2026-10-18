import { describe, expect, it } from 'vitest';
import { decodeEntities, stripHtml } from '$lib/html';

describe('decodeEntities', () => {
	it('decodes named entities used in the data files', () => {
		expect(decodeEntities('Beullens &amp; Hess')).toBe('Beullens & Hess');
		expect(decodeEntities('GiB&ndash;PiB')).toBe('GiB–PiB');
		expect(decodeEntities('&lt;tag&gt; &quot;q&quot;')).toBe('<tag> "q"');
	});

	it('decodes numeric references', () => {
		expect(decodeEntities('na&#239;ve')).toBe('naïve');
		expect(decodeEntities('na&#xEF;ve')).toBe('naïve');
	});

	it('leaves unknown or malformed entities untouched', () => {
		expect(decodeEntities('&bogus; 2^64 & Q')).toBe('&bogus; 2^64 & Q');
		expect(decodeEntities('&#x110000;')).toBe('&#x110000;');
	});
});

describe('stripHtml', () => {
	it('drops tags but keeps their text', () => {
		expect(stripHtml('see <a href="https://example.org/p.pdf">paper</a> for details')).toBe(
			'see paper for details'
		);
	});

	it('decodes entities after stripping tags', () => {
		expect(stripHtml('Straznickas &amp; Weis (<a href="#">paper</a>)')).toBe(
			'Straznickas & Weis (paper)'
		);
	});

	it('collapses whitespace and trims', () => {
		expect(stripHtml('  a\n\tb   c ')).toBe('a b c');
	});

	it('leaves plain strings alone', () => {
		expect(stripHtml('Fix is straightforward.')).toBe('Fix is straightforward.');
	});
});
