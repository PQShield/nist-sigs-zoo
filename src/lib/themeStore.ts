import { browser } from '$app/environment';
import { writable } from 'svelte/store';

type Theme = 'light' | 'dark' | 'system';

function createThemeStore() {
	const stored = browser ? (localStorage.getItem('theme') as Theme | null) : null;
	const initial: Theme = stored ?? 'system';

	const { subscribe, set } = writable<Theme>(initial);

	function apply(theme: Theme) {
		if (!browser) return;
		const prefersDark = matchMedia('(prefers-color-scheme: dark)').matches;
		const dark = theme === 'dark' || (theme === 'system' && prefersDark);
		document.documentElement.classList.toggle('dark', dark);
		localStorage.setItem('theme', theme);
	}

	return {
		subscribe,
		set(theme: Theme) {
			set(theme);
			apply(theme);
		},
		toggle() {
			// cycle: system → light → dark → system
			let current: Theme = 'system';
			const unsub = subscribe((v) => { current = v; });
			unsub();
			const next: Theme = current === 'system' ? 'light' : current === 'light' ? 'dark' : 'system';
			this.set(next);
		},
		init() {
			if (!browser) return;
			// Re-apply on system preference changes
			const mq = matchMedia('(prefers-color-scheme: dark)');
			mq.addEventListener('change', () => {
				let current: Theme = 'system';
				const unsub = subscribe((v) => { current = v; });
				unsub();
				if (current === 'system') apply('system');
			});
			// Apply the stored value (FOUC script already handled initial render)
			let current: Theme = 'system';
			const unsub = subscribe((v) => { current = v; });
			unsub();
			apply(current);
		},
	};
}

export const themeStore = createThemeStore();
