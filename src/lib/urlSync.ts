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

export interface UrlSyncEnv {
	location: { pathname: string; hash: string; search: string };
	isNavigating: () => boolean;
	goto: (target: string) => void;
}

/**
 * Debounced filter → URL sync of the query string `qs` (`replaceState` `goto` in `env`).
 * A sync that falls due while a navigation is in progress is rescheduled, not dropped, so the
 * URL catches up once the navigation settles (a navigation away unmounts the page, whose effect
 * cleanup calls the returned cancel function). Returns that cancel function.
 */
export function scheduleUrlSync(qs: string, env: UrlSyncEnv, delay = 300): () => void {
	let timer: ReturnType<typeof setTimeout>;
	const run = () => {
		if (env.isNavigating()) {
			timer = setTimeout(run, delay);
			return;
		}
		const { pathname, hash, search } = env.location;
		const target = urlSyncTarget(qs, pathname, hash, search, false);
		if (target !== null) env.goto(target);
	};
	timer = setTimeout(run, delay);
	return () => clearTimeout(timer);
}
