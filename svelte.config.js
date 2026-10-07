import adapter from '@sveltejs/adapter-static';

/** @type {import('@sveltejs/kit').Config} */
const config = {
	compilerOptions: {
		// Force runes mode for the project, except for libraries. Can be removed in svelte 6.
		runes: ({ filename }) => filename.split(/[/\\]/).includes('node_modules') ? undefined : true
	},
	kit: {
		adapter: adapter({
			pages: 'dist',
			assets: 'dist',
			fallback: undefined,
			precompress: false,
		}),
		paths: {
			base: process.env.BASE_PATH ?? '',
		},
		prerender: {
			// round-1/ is copied into dist/ post-build, not part of the SvelteKit app
			handleHttpError: ({ path, message }) => {
				if (path.startsWith('/round-1')) return;
				throw new Error(message);
			},
		},
	}
};

export default config;
