<script lang="ts">
	import { base } from '$app/paths';
	import { fmtDate } from '$lib/dates';
	import { EVENT_LABELS, type SchemeEvent, type SchemeEventKind } from '$lib/schemeHistory';

	let { events }: { events: SchemeEvent[] } = $props();

	const TONE: Record<SchemeEventKind, string> = {
		version: 'bg-pqs-ashgray text-pqs-midnight dark:bg-pqs-steel/40 dark:text-pqs-smoke',
		update: 'bg-blue-100 text-blue-800 dark:bg-blue-900/40 dark:text-blue-300',
		attack: 'bg-red-100 text-red-800 dark:bg-red-900/40 dark:text-red-300',
		standardization: 'bg-green-100 text-green-800 dark:bg-green-900/40 dark:text-green-300',
		milestone: 'bg-purple-100 text-purple-800 dark:bg-purple-900/40 dark:text-purple-300'
	};
</script>

<section aria-labelledby="history-heading" class="mt-10">
	<h2 id="history-heading" class="font-heading text-xl font-bold text-pqs-midnight dark:text-pqs-smoke">History</h2>
	{#if events.length === 0}
		<p class="mt-3 text-sm text-pqs-steel dark:text-pqs-bluegray">No events recorded for this scheme yet.</p>
	{:else}
		<ol class="mt-3">
			{#each events as event, i (i)}
				<li class="grid gap-1 border-t border-pqs-ashgray py-4 sm:grid-cols-[8rem_minmax(0,1fr)] sm:gap-6 dark:border-pqs-steel">
					<div>
						<time datetime={event.date} class="font-mono text-xs text-pqs-steel dark:text-pqs-bluegray">{fmtDate(event.date)}</time>
						<span class="mt-1 block w-fit rounded px-1.5 py-0.5 font-heading text-xs font-semibold {TONE[event.kind]}">{EVENT_LABELS[event.kind]}</span>
					</div>
					<div class="text-sm text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-apricot [&_a]:hover:underline">
						{#if event.title}<p class="font-heading font-semibold">{event.title}</p>{/if}
						{#if event.html}<p class="mt-1">{@html event.html}</p>{/if}
					</div>
				</li>
			{/each}
		</ol>
	{/if}
	<p class="mt-2 text-sm"><a href="{base}/timeline/" class="text-pqs-apricot hover:underline">Full timeline →</a></p>
</section>
