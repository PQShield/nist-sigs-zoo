import { resolve } from '$app/paths';

/**
 * The base path without a trailing slash, as `base` from `$app/paths` was before SvelteKit 3:
 * relative to the page while prerendering, absolute in the browser. Call it while a component
 * initialises, not at module level, so the prerendered value matches the page.
 */
export function basePath(): string {
	return resolve('/').slice(0, -1);
}
