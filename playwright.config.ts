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
		command: 'corepack npm run build && corepack npm run preview -- --port 4175',
		url: 'http://localhost:4175',
		reuseExistingServer: true,
		timeout: 90_000,
	},
});
