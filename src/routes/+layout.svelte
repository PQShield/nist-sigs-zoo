<script lang="ts">
	import './layout.css';
	import { onMount } from 'svelte';
	import { page } from '$app/stores';
	import { base } from '$app/paths';
	import { themeStore } from '$lib/themeStore';
	import { lastUpdated } from '$lib/schemeData';
	import { SITE_NAME } from '$lib/constants';

	let { children } = $props();

	const themeIcon = $derived(
		$themeStore === 'dark' ? '🌙' : $themeStore === 'light' ? '☀️' : '🖥️'
	);
	const themeLabel = $derived(`Theme: ${$themeStore} — click to cycle`);

	const path = $derived($page.url.pathname.slice(base.length));

	const NAV = [
		{ href: 'signatures/', label: 'Signatures' },
		{ href: 'kems/', label: 'KEMs' },
		{ href: 'programmes/', label: 'Programmes' },
		{ href: 'timeline/', label: 'Timeline' }
	];

	let menuOpen = $state(false);

	onMount(() => {
		themeStore.init();
	});

	function closeMenu() {
		menuOpen = false;
	}
</script>

<div class="flex min-h-screen flex-col bg-pqs-smoke text-pqs-midnight dark:bg-pqs-midnight dark:text-pqs-smoke">
	<header class="bg-pqs-midnight shadow-md">
		<nav class="mx-auto flex max-w-screen-2xl items-center gap-6 px-6 py-4">
			<a href="{base}/" class="flex items-center gap-2 text-white" onclick={closeMenu}>
				<span class="font-heading text-xl font-bold tracking-tight">PQShield</span>
				<span class="rounded bg-pqs-apricot px-2 py-0.5 font-heading text-xs font-semibold text-pqs-midnight">
					{SITE_NAME}
				</span>
			</a>

			<!-- Desktop nav -->
			<div class="ml-auto hidden items-center gap-5 font-heading text-sm md:flex">
				{#each NAV as item (item.href)}
					<a
						href="{base}/{item.href}"
						aria-current={path.startsWith('/' + item.href) ? 'page' : undefined}
						class="transition-colors hover:text-pqs-apricot {path.startsWith('/' + item.href) ? 'text-white' : 'text-pqs-bluegray'}"
					>
						{item.label}
					</a>
				{/each}
				<a
					href="https://github.com/pqshield/nist-sigs-zoo"
					target="_blank"
					rel="noopener noreferrer"
					class="text-pqs-bluegray hover:text-pqs-apricot transition-colors"
				>
					Contribute ↗
				</a>
				<button
					onclick={() => themeStore.toggle()}
					aria-label={themeLabel}
					title={themeLabel}
					class="rounded px-2 py-1 text-base text-pqs-bluegray hover:text-white hover:bg-pqs-steel transition-colors"
				>
					{themeIcon}
				</button>
			</div>

			<!-- Mobile: theme toggle always visible + hamburger -->
			<div class="ml-auto flex items-center gap-2 md:hidden">
				<button
					onclick={() => themeStore.toggle()}
					aria-label={themeLabel}
					title={themeLabel}
					class="rounded px-2 py-1 text-base text-pqs-bluegray hover:text-white hover:bg-pqs-steel transition-colors"
				>
					{themeIcon}
				</button>
				<button
					onclick={() => (menuOpen = !menuOpen)}
					aria-label="Toggle menu"
					aria-expanded={menuOpen}
					class="rounded p-1.5 text-pqs-bluegray hover:text-white hover:bg-pqs-steel transition-colors"
				>
					{#if menuOpen}
						<svg xmlns="http://www.w3.org/2000/svg" class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
							<path stroke-linecap="round" stroke-linejoin="round" d="M6 18L18 6M6 6l12 12" />
						</svg>
					{:else}
						<svg xmlns="http://www.w3.org/2000/svg" class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
							<path stroke-linecap="round" stroke-linejoin="round" d="M4 6h16M4 12h16M4 18h16" />
						</svg>
					{/if}
				</button>
			</div>
		</nav>

		<!-- Mobile dropdown -->
		{#if menuOpen}
			<div class="border-t border-pqs-steel/30 px-6 pb-4 pt-3 font-heading text-sm md:hidden">
				<div class="flex flex-col gap-3">
					{#each NAV as item (item.href)}
						<a
							href="{base}/{item.href}"
							onclick={closeMenu}
							aria-current={path.startsWith('/' + item.href) ? 'page' : undefined}
							class="transition-colors hover:text-pqs-apricot {path.startsWith('/' + item.href) ? 'text-white' : 'text-pqs-bluegray'}"
						>
							{item.label}
						</a>
					{/each}
					<a
						href="https://github.com/pqshield/nist-sigs-zoo"
						target="_blank"
						rel="noopener noreferrer"
						onclick={closeMenu}
						class="text-pqs-bluegray hover:text-pqs-apricot transition-colors"
					>
						Contribute ↗
					</a>
				</div>
			</div>
		{/if}
	</header>

	<main class="flex-1">
		{@render children()}
	</main>

	<footer class="mt-8 border-t border-pqs-bluegray/30 bg-pqs-midnight px-6 py-6 text-xs text-pqs-bluegray dark:border-pqs-steel">
		<div class="mx-auto max-w-screen-2xl space-y-1">
			<p>
        Built by Thom Wiggers / PQShield.
				Data licensed under
				<a href="{base}/LICENSE.md" class="text-pqs-apricot hover:underline">CC BY-SA 4.0</a>.
        Most recent scheme data is dated {lastUpdated}.
			</p>
			<p>
				<a
					href="https://github.com/pqshield/nist-sigs-zoo"
					target="_blank"
					rel="noopener noreferrer"
					class="text-pqs-apricot hover:underline">GitHub</a
				>
				·
				<a
					href="https://bench.cr.yp.to"
					target="_blank"
					rel="noopener noreferrer"
					class="text-pqs-apricot hover:underline">eBACS</a
				>: more comprehensive benchmarks on more platforms
			</p>
		</div>
	</footer>
</div>
