import adapter from '@sveltejs/adapter-static';
import tailwindcss from '@tailwindcss/vite';
import { sveltekit } from '@sveltejs/kit/vite';
import { defineConfig } from 'vite';
import * as yaml from 'js-yaml';

function yamlPlugin() {
	return {
		name: 'yaml-loader',
		transform(src: string, id: string) {
			if (id.endsWith('.yaml') || id.endsWith('.yml')) {
				const parsed = yaml.load(src);
				return {
					code: `export default ${JSON.stringify(parsed)}`,
					map: null
				};
			}
		}
	};
}

export default defineConfig({
	plugins: [
		yamlPlugin(),
		tailwindcss(),
		sveltekit({
			compilerOptions: {
				// Force runes mode for the project, except for libraries. Can be removed in svelte 6.
				runes: ({ filename }) => (filename.split(/[/\\]/).includes('node_modules') ? undefined : true)
			},
			adapter: adapter({
				pages: 'dist',
				assets: 'dist',
				fallback: undefined,
				precompress: false
			}),
			paths: {
				base: (process.env.BASE_PATH ?? '') as '' | `/${string}`
			},
			prerender: {
				// round-1/ is copied into dist/ post-build, not part of the SvelteKit app
				handleHttpError: ({ path, message }) => {
					if (path.startsWith('/round-1')) return;
					throw new Error(message);
				}
			}
		})
	],
	build: { chunkSizeWarningLimit: 1000 }
});
