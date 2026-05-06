<script lang="ts">
	import './layout.css';
	import { onMount } from 'svelte';
	import { themeStore } from '$lib/themeStore';

	let { children } = $props();

	const themeIcon = $derived(
		$themeStore === 'dark' ? '🌙' : $themeStore === 'light' ? '☀️' : '🖥️'
	);
	const themeLabel = $derived(`Theme: ${$themeStore} — click to cycle`);

	onMount(() => {
		themeStore.init();
	});
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
				<a
					href="/round-1/"
					class="text-pqs-bluegray hover:text-pqs-apricot transition-colors"
				>
					Round 1
				</a>
				<a
					href="https://github.com/thomwiggers/nist-sigs-zoo"
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
				Data licensed under
				<a href="/LICENSE.md" class="text-pqs-apricot hover:underline">CC BY-SA 4.0</a>. Last updated 2024-10-28.
			</p>
			<p>
				<a href="/round-1/" class="text-pqs-apricot hover:underline">Round 1 snapshot</a>
				·
				<a
					href="https://github.com/thomwiggers/nist-sigs-zoo"
					target="_blank"
					rel="noopener noreferrer"
					class="text-pqs-apricot hover:underline">GitHub</a
				>
			</p>
		</div>
	</footer>
</div>
