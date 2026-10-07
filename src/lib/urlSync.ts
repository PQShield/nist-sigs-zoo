/**
 * Target for a debounced filter-state URL sync, or null when the sync must be skipped.
 *
 * Skipped when the query string is already what the URL holds (the mount-time sync
 * would otherwise be a no-op `goto`, which still cancels an in-flight in-app
 * navigation, e.g. a click on a scheme link) or when a navigation is in progress.
 */
export function urlSyncTarget(
	qs: string,
	pathname: string,
	hash: string,
	currentSearch: string,
	isNavigating: boolean
): string | null {
	if (isNavigating) return null;
	const wanted = qs ? `?${qs}` : '';
	const current = currentSearch === '?' ? '' : currentSearch;
	if (wanted === current) return null;
	return (qs ? wanted : pathname) + hash;
}
