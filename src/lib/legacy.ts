/** Query keys of the signature page's URL codec (SIGNATURE_FILTER + compareFilters.ts) and round. */
export const SIGNATURE_PARAMS = [
	'r', 's', 'l', 'nl', 'p', 'ps', 'hb', 'hi', 'hw', 'v',
	'pkMin', 'pkMax', 'sigMin', 'sigMax', 'psMin', 'psMax',
	'scMin', 'scMax', 'vcMin', 'vcMax', 'suMin', 'suMax', 'vuMin', 'vuMax',
	'sort', 'dir'
] as const;

export function legacyPathRedirect(target: string, search: string, hash: string, base: string): string {
	return `${base}/${target}${search}${hash}`;
}

export function legacyHomeRedirect(search: string, hash: string, base: string): string | null {
	const params = new URLSearchParams(search);
	const keys = [...params.keys()];
	if (!keys.some((k) => (SIGNATURE_PARAMS as readonly string[]).includes(k))) return null;
	return legacyPathRedirect('signatures/', search, hash, base);
}
