<script lang="ts">
	import './layout.css';
	import { onMount } from 'svelte';
	import { page } from '$app/stores';
	import { themeStore } from '$lib/themeStore';
	import { roundStore, type Round } from '$lib/roundStore';
	import { lastUpdated } from '$lib/schemeData';

	let { children } = $props();

	const themeIcon = $derived(
		$themeStore === 'dark' ? '🌙' : $themeStore === 'light' ? '☀️' : '🖥️'
	);
	const themeLabel = $derived(`Theme: ${$themeStore} — click to cycle`);

	const isHome = $derived($page.route.id === '/');

	onMount(() => {
		themeStore.init();
	});

	function setRound(r: Round) {
		roundStore.set(r);
	}
</script>

<div class="flex min-h-screen flex-col bg-pqs-smoke text-pqs-midnight dark:bg-pqs-midnight dark:text-pqs-smoke">
	<header class="bg-pqs-midnight shadow-md">
		<nav class="mx-auto flex max-w-screen-2xl items-center gap-6 px-6 py-4">
			<a href="/" class="flex items-center gap-2 text-white">
				<span class="font-heading text-xl font-bold tracking-tight">PQShield</span>
				<span class="rounded bg-pqs-apricot px-2 py-0.5 font-heading text-xs font-semibold text-pqs-midnight">
					NIST Signatures Zoo
				</span>
			</a>
			<div class="ml-auto flex items-center gap-5 font-heading text-sm">
				{#if isHome}
					<div class="flex items-center gap-1 rounded border border-pqs-steel/40 p-0.5 font-heading text-sm">
						<button
							onclick={() => setRound('latest')}
							class="rounded px-2 py-0.5 transition-colors {$roundStore === 'latest' ? 'bg-pqs-apricot text-pqs-midnight font-semibold' : 'text-pqs-bluegray hover:text-white'}"
						>
							Latest
						</button>
						<button
							onclick={() => setRound('round-2')}
							class="rounded px-2 py-0.5 transition-colors {$roundStore === 'round-2' ? 'bg-pqs-apricot text-pqs-midnight font-semibold' : 'text-pqs-bluegray hover:text-white'}"
						>
							Round 2
						</button>
						<button
							onclick={() => setRound('round-1')}
							class="rounded px-2 py-0.5 transition-colors {$roundStore === 'round-1' ? 'bg-pqs-apricot text-pqs-midnight font-semibold' : 'text-pqs-bluegray hover:text-white'}"
						>
							Round 1
						</button>
					</div>
				{/if}
				<a href="/history/" class="text-pqs-bluegray hover:text-pqs-apricot transition-colors">
					History
				</a>
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
		</nav>
	</header>

	<main class="flex-1">
		{@render children()}
	</main>

	<footer class="mt-8 border-t border-pqs-bluegray/30 bg-pqs-midnight px-6 py-6 text-xs text-pqs-bluegray dark:border-pqs-steel">
		<div class="mx-auto max-w-screen-2xl space-y-1">
			<p>
        Built by Thom Wiggers / PQShield.
				Data licensed under
				<a href="/LICENSE.md" class="text-pqs-apricot hover:underline">CC BY-SA 4.0</a>.
        Most recent scheme data is dated {lastUpdated}.
			</p>
			<p>
				<a
					href="https://github.com/pqshield/nist-sigs-zoo"
					target="_blank"
					rel="noopener noreferrer"
					class="text-pqs-apricot hover:underline">GitHub</a
				>
			</p>
		</div>
	</footer>
</div>
