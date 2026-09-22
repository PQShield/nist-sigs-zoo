/**
 * Helpers for the security-flag strings in `data/schemes/*.yaml` and
 * `data/kems/*.yaml`, which may contain inline HTML (links, entities).
 */

const NAMED_ENTITIES: Record<string, string> = {
	amp: '&',
	lt: '<',
	gt: '>',
	quot: '"',
	apos: "'",
	nbsp: ' ',
	ndash: '–',
	mdash: '—',
	hellip: '…',
	lsquo: '‘',
	rsquo: '’',
	ldquo: '“',
	rdquo: '”',
	times: '×',
	le: '≤',
	ge: '≥',
	deg: '°'
};

/** Decode the HTML entities used in the data files. */
export function decodeEntities(text: string): string {
	return text.replace(/&(#x[0-9a-fA-F]+|#[0-9]+|[a-zA-Z][a-zA-Z0-9]*);/g, (match, ref: string) => {
		if (ref[0] === '#') {
			const code =
				ref[1] === 'x' || ref[1] === 'X'
					? Number.parseInt(ref.slice(2), 16)
					: Number.parseInt(ref.slice(1), 10);
			if (!Number.isFinite(code) || code <= 0 || code > 0x10ffff) return match;
			return String.fromCodePoint(code);
		}
		const named = NAMED_ENTITIES[ref.toLowerCase()];
		return named ?? match;
	});
}

/**
 * Flatten a flag string to plain text: drop tags, decode entities, collapse
 * whitespace. For contexts that cannot render HTML (Vega tooltips, aria labels).
 */
export function stripHtml(text: string): string {
	return decodeEntities(text.replace(/<[^>]*>/g, ''))
		.replace(/\s+/g, ' ')
		.trim();
}
