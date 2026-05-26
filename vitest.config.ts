import { defineConfig } from 'vitest/config';
import yaml from 'js-yaml';
import path from 'path';

function yamlPlugin() {
	return {
		name: 'yaml-loader',
		transform(src: string, id: string) {
			if (id.endsWith('.yaml') || id.endsWith('.yml')) {
				const parsed = yaml.load(src);
				return { code: `export default ${JSON.stringify(parsed)}`, map: null };
			}
		},
	};
}

export default defineConfig({
	plugins: [yamlPlugin()],
	test: {
		include: ['src/**/*.test.ts'],
		environment: 'node',
	},
	resolve: {
		alias: {
			'$lib': path.resolve('./src/lib'),
		},
	},
});
