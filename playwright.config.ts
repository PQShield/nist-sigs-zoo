import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
	testDir: './e2e',
	// One local retry (two in CI) as a safety net for transient navigation/render
	// flakiness under load; the specs are written to avoid races, not rely on this.
	retries: process.env.CI ? 2 : 1,
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
