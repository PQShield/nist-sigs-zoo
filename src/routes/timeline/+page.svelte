<script lang="ts">
	import { onMount } from 'svelte';
	import { goto } from '$app/navigation';
	import { navigating } from '$app/state';
	import { base } from '$app/paths';
	import { SITE_NAME } from '$lib/constants';
	import ProgrammeChips from '$lib/components/ProgrammeChips.svelte';
	import { FILTER_HEADING, FILTER_OPTION, FILTER_OPTION_TEXT } from '$lib/components/filterStyles';
	import { fmtDate } from '$lib/dates';
	import { schemeHref } from '$lib/slugs';
	import { scheduleUrlSync } from '$lib/urlSync';
	import { slugOf } from '$lib/schemeIndex';
	import {
		decodeTimelineFilter,
		defaultTimelineFilter,
		encodeTimelineFilter,
		entryChips,
		filterTimeline,
		HISTORY_TYPE_LABELS,
		timelineSummary,
		type TimelineFilter
	} from '$lib/timeline';
	import type { HistoryType } from '$lib/types';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();

	const programmeIds = $derived(data.programmeOptions.map((o) => o.id));

	// The prerendered page shows everything; the URL's filter is applied once hydrated.
	// svelte-ignore state_referenced_locally
	let filter = $state<TimelineFilter>(
		defaultTimelineFilter(
			data.programmeOptions.map((o) => o.id),
			data.types
		)
	);
	let ready = $state(false);
	const shown = $derived(filterTimeline(data.items, filter));

	onMount(() => {
		filter = decodeTimelineFilter(new URLSearchParams(location.search), programmeIds, data.types);
		ready = true;
	});

	$effect(() => {
		if (!ready) return;
		const qs = encodeTimelineFilter(filter, programmeIds, data.types).toString();
		return scheduleUrlSync(qs, {
			location: window.location,
			isNavigating: () => navigating.to !== null,
			goto: (target) => goto(target, { replaceState: true, keepFocus: true, noScroll: true })
		});
	});

	function toggleProgramme(id: string) {
		const next = new Set(filter.programmes);
		if (next.has(id)) next.delete(id);
		else next.add(id);
		filter = { ...filter, programmes: next };
	}

	function toggleType(type: HistoryType) {
		const next = new Set(filter.types);
		if (next.has(type)) next.delete(type);
		else next.add(type);
		filter = { ...filter, types: next };
	}

	function showAll() {
		filter = defaultTimelineFilter(programmeIds, data.types);
	}

	const TYPE_BADGE: Record<HistoryType, string> = {
		update: 'bg-blue-100 text-blue-800 dark:bg-blue-900/40 dark:text-blue-300',
		attack: 'bg-red-100 text-red-800 dark:bg-red-900/40 dark:text-red-300',
		standardization: 'bg-green-100 text-green-800 dark:bg-green-900/40 dark:text-green-300',
		milestone: 'bg-purple-100 text-purple-800 dark:bg-purple-900/40 dark:text-purple-300'
	};
</script>

<svelte:head>
	<title>Timeline · {SITE_NAME}</title>
</svelte:head>

<div class="mx-auto max-w-screen-lg px-6 py-10">
	<h1 class="font-heading text-2xl font-bold text-pqs-midnight dark:text-pqs-smoke">Timeline</h1>
	<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
		Scheme data updates, attacks and programme milestones tracked on this site.
	</p>

	<div class="mt-8 grid gap-8 md:grid-cols-[200px_1fr]">
		<form aria-label="Filter events" onsubmit={(e) => e.preventDefault()}>
			<fieldset class="mb-5">
				<legend class={FILTER_HEADING}>Programme</legend>
				<div class="space-y-0.5">
					{#each data.programmeOptions as option (option.id)}
						<label class={FILTER_OPTION}>
							<input
								type="checkbox"
								class="accent-pqs-apricot"
								checked={filter.programmes.has(option.id)}
								onchange={() => toggleProgramme(option.id)}
							/>
							<span class={FILTER_OPTION_TEXT}>{option.label}</span>
						</label>
					{/each}
				</div>
			</fieldset>
			<fieldset class="mb-5">
				<legend class={FILTER_HEADING}>Event type</legend>
				<div class="space-y-0.5">
					{#each data.types as type (type)}
						<label class={FILTER_OPTION}>
							<input
								type="checkbox"
								class="accent-pqs-apricot"
								checked={filter.types.has(type)}
								onchange={() => toggleType(type)}
							/>
							<span class={FILTER_OPTION_TEXT}>{HISTORY_TYPE_LABELS[type]}</span>
						</label>
					{/each}
				</div>
			</fieldset>
			<button
				type="button"
				onclick={showAll}
				class="font-heading text-sm font-semibold text-pqs-steel underline hover:text-pqs-apricot dark:text-pqs-apricot"
				>Show all events</button
			>
		</form>

		<div>
			<p role="status" class="text-sm text-pqs-steel dark:text-pqs-bluegray">
				{timelineSummary(shown.length, data.items.length)}
			</p>

			{#if shown.length === 0}
				<p class="mt-6 text-sm text-pqs-midnight dark:text-pqs-smoke">No events match these filters.</p>
			{/if}

			<ol aria-label="Events" class="mt-6 space-y-0">
				{#each shown as item (item.key)}
					<li class="relative flex gap-6 pb-8 last:pb-0">
						<!-- spine -->
						<div class="flex flex-col items-center" aria-hidden="true">
							<div
								class="mt-1 h-3 w-3 shrink-0 rounded-full border-2 border-pqs-apricot bg-pqs-smoke dark:bg-pqs-midnight"
							></div>
							<div class="mt-1 w-px flex-1 bg-pqs-ashgray dark:bg-pqs-steel/40"></div>
						</div>
						<!-- content -->
						<div class="min-w-0 pb-2">
							<div class="flex flex-wrap items-center gap-2">
								<time datetime={item.entry.date} class="font-mono text-xs text-pqs-steel dark:text-pqs-bluegray"
									>{fmtDate(item.entry.date)}</time
								>
								<span class="rounded px-1.5 py-0.5 font-heading text-xs font-semibold {TYPE_BADGE[item.entry.type]}">
									{HISTORY_TYPE_LABELS[item.entry.type]}
								</span>
								<span class="sr-only">Programmes:</span>
								<ProgrammeChips link chips={entryChips(item.programmes, data.programmeOptions)} />
							</div>
							<p
								class="mt-1 text-sm text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-steel [&_a]:underline dark:[&_a]:text-pqs-apricot"
							>
								{@html item.entry.description}
							</p>
							{#if item.entry.schemes && item.entry.schemes.length}
								{@const names = item.entry.schemes}
								<p class="mt-1 text-xs text-pqs-steel dark:text-pqs-bluegray">
									<span class="sr-only">Schemes:</span>
									{#each names as name, i (name)}
										<a href={schemeHref(base, slugOf(name))} class="underline hover:text-pqs-apricot">{name}</a>{i <
										names.length - 1
											? ', '
											: ''}
									{/each}
								</p>
							{/if}
						</div>
					</li>
				{/each}
			</ol>
		</div>
	</div>
</div>
