<script lang="ts">
	import type { HistoryEntry } from '$lib/types';

	let { data }: { data: { entries: HistoryEntry[] } } = $props();

	const typeLabel: Record<string, string> = {
		update: 'Update',
		attack: 'Attack',
		standardization: 'Standardization',
		milestone: 'Milestone'
	};

	const typeBadge: Record<string, string> = {
		update: 'bg-blue-100 text-blue-800 dark:bg-blue-900/40 dark:text-blue-300',
		attack: 'bg-red-100 text-red-800 dark:bg-red-900/40 dark:text-red-300',
		standardization: 'bg-green-100 text-green-800 dark:bg-green-900/40 dark:text-green-300',
		milestone: 'bg-purple-100 text-purple-800 dark:bg-purple-900/40 dark:text-purple-300'
	};
</script>

<svelte:head>
	<title>History — NIST Signatures Zoo</title>
</svelte:head>

<div class="mx-auto max-w-screen-md px-6 py-10">
	<h1 class="font-heading text-2xl font-bold text-pqs-midnight dark:text-pqs-smoke">History</h1>
	<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
		Timeline of scheme data updates and security events tracked on this site.
	</p>

	<ol class="mt-8 space-y-0">
		{#each data.entries as entry (entry.date + entry.type + entry.description)}
			<li class="relative flex gap-6 pb-8 last:pb-0">
				<!-- spine -->
				<div class="flex flex-col items-center">
					<div class="mt-1 h-3 w-3 shrink-0 rounded-full border-2 border-pqs-apricot bg-pqs-smoke dark:bg-pqs-midnight"></div>
					<div class="mt-1 w-px flex-1 bg-pqs-ashgray dark:bg-pqs-steel/40 last:hidden"></div>
				</div>
				<!-- content -->
				<div class="pb-2">
					<div class="flex flex-wrap items-center gap-2">
						<time class="font-mono text-xs text-pqs-steel dark:text-pqs-bluegray">{entry.date}</time>
						<span class="rounded px-1.5 py-0.5 font-heading text-xs font-semibold {typeBadge[entry.type]}">
							{typeLabel[entry.type]}
						</span>
					</div>
					<p class="mt-1 text-sm text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-apricot [&_a]:hover:underline">{@html entry.description}</p>
					{#if entry.schemes && entry.schemes.length}
						<p class="mt-1 text-xs text-pqs-steel dark:text-pqs-bluegray">
							{entry.schemes.join(', ')}
						</p>
					{/if}
				</div>
			</li>
		{/each}
	</ol>
</div>
