import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
	testDir: './e2e',
	use: {
		baseURL: 'http://localhost:4175',
	},
	projects: [
		{ name: 'chromium', use: { ...devices['Desktop Chrome'] } },
	],
	webServer: {
		// CI pre-builds before running tests; locally reuse whatever server is already running
		command: process.env.CI
			? 'npm run preview -- --port 4175'
			: 'corepack npm run build && corepack npm run preview -- --port 4175',
		url: 'http://localhost:4175',
		reuseExistingServer: !process.env.CI,
		timeout: 30_000,
	},
});
