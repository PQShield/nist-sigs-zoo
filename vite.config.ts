import tailwindcss from '@tailwindcss/vite';
import { sveltekit } from '@sveltejs/kit/vite';
import { defineConfig } from 'vite';
import yaml from 'js-yaml';

function yamlPlugin() {
	return {
		name: 'yaml-loader',
		transform(src: string, id: string) {
			if (id.endsWith('.yaml') || id.endsWith('.yml')) {
				const parsed = yaml.load(src);
				return {
					code: `export default ${JSON.stringify(parsed)}`,
					map: null,
				};
			}
		},
	};
}

export default defineConfig({
	plugins: [yamlPlugin(), tailwindcss(), sveltekit()],
	build: { chunkSizeWarningLimit: 1000 },
});
