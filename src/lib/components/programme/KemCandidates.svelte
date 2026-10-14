<script lang="ts">
	import { untrack } from 'svelte';
	import { base } from '$app/paths';
	import { presentProgrammeGroups } from '$lib/compareFilters';
	import { computeKemRanges } from '$lib/kemData';
	import { defaultCompareFilter, filterCompareRows } from '$lib/compare';
	import { KEM_FILTER } from '$lib/primitives';
	import { allKemData } from '$lib/kemSchemeData';
	import {
		anyKemTimings,
		compareHref,
		focusTarget,
		ngccTargets,
		programmeKemRows,
		unmappedAtTarget
	} from '$lib/programmePage';
	import type { NgccLevel, ProgrammeYaml } from '$lib/types';
	import ChartModeToggle from '../ChartModeToggle.svelte';
	import KemScatterPlot from '../KemScatterPlot.svelte';
	import KemTable from '../KemTable.svelte';
	import SegmentedToggle from '../SegmentedToggle.svelte';

	let { programme }: { programme: ProgrammeYaml } = $props();

	const p = untrack(() => programme);
	const rows = programmeKemRows(p.id, allKemData);
	const targets = ngccTargets(rows);
	const timed = anyKemTimings(rows);
	const defaults = defaultCompareFilter(
		KEM_FILTER,
		computeKemRanges(rows),
		new Set(rows.map((r) => r.scheme)),
		presentProgrammeGroups(rows)
	);
	const firstTarget = targets[0] ?? null;

	let target = $state<NgccLevel | null>(firstTarget);
	let filter = $state(focusTarget(defaults, firstTarget));

	const filtered = $derived(filterCompareRows(KEM_FILTER, rows, filter));
	const unmapped = $derived(target === null ? [] : unmappedAtTarget(rows, target));

	function chooseTarget(next: NgccLevel) {
		target = next;
		filter = focusTarget(filter, next);
	}
</script>

{#if rows.length === 0}
	<p class="rounded border border-dashed border-pqs-bluegray px-4 py-6 text-sm text-pqs-steel dark:border-pqs-steel dark:text-pqs-bluegray">
		No KEMs from this programme are in our data yet.
	</p>
{:else}
	<div class="space-y-4">
		{#if targets.length > 1}
			<SegmentedToggle
				label="Security target"
				options={targets.map((t) => ({ value: t, label: `${t}-bit` }))}
				value={target ?? targets[0]}
				onchange={chooseTarget}
			/>
		{/if}

		<div class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
			<div class="mb-3 flex flex-wrap items-center justify-between gap-3">
				<h3 class="font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					{filter.chart === 'speed' ? 'Encapsulation vs. decapsulation time' : 'pk size vs. ciphertext size'}
					<span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h3>
				{#if timed}
					<ChartModeToggle mode={filter.chart} onchange={(chart) => (filter = { ...filter, chart })} />
				{/if}
			</div>
			<KemScatterPlot
				rows={filtered}
				xField={filter.chart === 'speed' ? 'encapsTime' : 'pk'}
				yField={filter.chart === 'speed' ? 'decapsTime' : 'ct'}
			/>
		</div>

		{#if !timed}
			<p role="note" class="text-xs text-pqs-steel dark:text-pqs-bluegray">
				Sizes only for now: none of these candidates is in our benchmark harness yet, so the table
				shows them as not yet benchmarked. Timings will follow once they are added.
			</p>
		{/if}

		<KemTable rows={filtered} />

		{#if unmapped.length > 0}
			<p class="text-xs text-pqs-steel dark:text-pqs-bluegray">
				No recommended {target}-bit parameter set in our data: {unmapped.join(', ')}.
			</p>
		{/if}

		<p class="text-right">
			<a href={compareHref(base, p.id, 'kem', { target })} class="font-heading text-sm font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot">
				Open in the KEM comparison →
			</a>
		</p>
	</div>
{/if}
