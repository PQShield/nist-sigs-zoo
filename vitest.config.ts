import { defineConfig } from 'vitest/config';
import * as yaml from 'js-yaml';

function yamlPlugin() {
	return {
		name: 'yaml-loader',
		transform(src: string, id: string) {
			if (id.endsWith('.yaml') || id.endsWith('.yml')) {
				const parsed = yaml.load(src);
				return { code: `export default ${JSON.stringify(parsed)}`, map: null };
			}
		}
	};
}

export default defineConfig({
	plugins: [yamlPlugin()],
	test: {
		include: ['src/**/*.test.ts'],
		environment: 'node'
	}
});
