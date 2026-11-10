import js from '@eslint/js';
import { defineConfig } from 'eslint/config';
import prettier from 'eslint-config-prettier';
import svelte from 'eslint-plugin-svelte';
import globals from 'globals';
import ts from 'typescript-eslint';
import svelteConfig from './svelte.config.js';

export default defineConfig(
	{
		ignores: [
			'node_modules/',
			'dist/',
			'build/',
			'.svelte-kit/',
			'test-results/',
			'playwright-report/',
			'round-1/',
			'data/',
			'static/',
			'bench/',
			'bench-kem/',
			'bench-common/',
			'.ngcc-out*/'
		]
	},
	js.configs.recommended,
	...ts.configs.recommended,
	...svelte.configs.recommended,
	prettier,
	...svelte.configs.prettier,
	{
		languageOptions: {
			globals: { ...globals.browser, ...globals.node }
		},
		rules: {
			// Internal links are built with `base` from $app/paths, which works with adapter-static + BASE_PATH.
			'svelte/no-navigation-without-resolve': 'off',
			// {@html} only renders repo-authored HTML from data/*.yaml, never user input.
			'svelte/no-at-html-tags': 'off',
			// {' · '} literals deliberately keep whitespace that Svelte trims at {#if} block boundaries.
			'svelte/no-useless-mustaches': 'off',
			// Filter Sets are immutable state: copied, mutated locally, then reassigned (never mutated in place).
			'svelte/prefer-svelte-reactivity': 'off'
		}
	},
	{
		files: ['**/*.svelte', '**/*.svelte.ts', '**/*.svelte.js'],
		languageOptions: {
			parserOptions: {
				projectService: true,
				extraFileExtensions: ['.svelte'],
				parser: ts.parser,
				svelteConfig
			}
		},
		rules: {
			// Bare reads inside $effect(() => { a; b; ... }) are how runes declare explicit dependencies.
			'@typescript-eslint/no-unused-expressions': 'off'
		}
	}
);
