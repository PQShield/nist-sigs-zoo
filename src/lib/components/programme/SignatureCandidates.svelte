<script lang="ts">
	import { untrack } from 'svelte';
	import { base } from '$app/paths';
	import { createFilterStore } from '$lib/filterStore';
	import {
		anySigTimings,
		compareHref,
		CURRENT_VIEW,
		focusTarget,
		ngccTargets,
		programmeSignatureRows,
		stageViews,
		unmappedAtTarget
	} from '$lib/programmePage';
	import { allSchemeData } from '$lib/schemeData';
	import { SIGNATURES } from '$lib/primitives';
	import type { NgccLevel, ProgrammeYaml } from '$lib/types';
	import ChartModeToggle from '../ChartModeToggle.svelte';
	import ScatterPlot from '../ScatterPlot.svelte';
	import CompareTable from '../CompareTable.svelte';
	import SegmentedToggle from '../SegmentedToggle.svelte';

	let { programme }: { programme: ProgrammeYaml } = $props();

	const p = untrack(() => programme);
	const views = stageViews(p, allSchemeData);
	const initial = programmeSignatureRows(p.id, allSchemeData, CURRENT_VIEW);
	const firstTarget = ngccTargets(initial.parameterSets)[0] ?? null;

	// ScatterPlot and SchemeTable read the shared filter store; this section owns it while mounted
	// (every compare page rebuilds it on load). createFilterStore() keeps the same store object.
	const { store, filteredRows } = createFilterStore(initial.parameterSets, initial.ranges, new URLSearchParams());
	store.update((f) => focusTarget(f, firstTarget));

	let view = $state(CURRENT_VIEW);
	let rows = $state(initial.parameterSets);
	let target = $state<NgccLevel | null>(firstTarget);

	const targets = $derived(ngccTargets(rows));
	const timed = $derived(anySigTimings(rows));
	const unmapped = $derived(target === null ? [] : unmappedAtTarget(rows, target));

	function chooseView(next: string) {
		view = next;
		const r = programmeSignatureRows(p.id, allSchemeData, next);
		rows = r.parameterSets;
		target = ngccTargets(r.parameterSets)[0] ?? null;
		// A stage change intentionally resets the other filters; one store write, no transient state.
		createFilterStore(r.parameterSets, r.ranges, new URLSearchParams(), (f) => focusTarget(f, target));
	}

	function chooseTarget(next: NgccLevel) {
		target = next;
		store.update((f) => focusTarget(f, next));
	}
</script>

{#if initial.parameterSets.length === 0}
	<p class="rounded border border-dashed border-pqs-bluegray px-4 py-6 text-sm text-pqs-steel dark:border-pqs-steel dark:text-pqs-bluegray">
		No signatures from this programme are in our data yet.
	</p>
{:else}
	<div class="space-y-4">
		{#if views.length > 0 || targets.length > 1}
			<div class="flex flex-wrap items-center gap-3">
				{#if views.length > 0}
					<SegmentedToggle label="Stage" options={views.map((v) => ({ value: v.id, label: v.label }))} value={view} onchange={chooseView} />
				{/if}
				{#if targets.length > 1}
					<SegmentedToggle
						label="Security target"
						options={targets.map((t) => ({ value: t, label: `${t}-bit` }))}
						value={target ?? targets[0]}
						onchange={chooseTarget}
					/>
				{/if}
			</div>
		{/if}

		<div class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
			<div class="mb-3 flex flex-wrap items-center justify-between gap-3">
				<h3 class="font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					{$store.chart === 'speed' ? 'Signing vs. verification time' : 'pk size vs. sig size'}
					<span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h3>
				{#if timed}
					<ChartModeToggle mode={$store.chart} onchange={(chart) => store.update((f) => ({ ...f, chart }))} />
				{/if}
			</div>
			<ScatterPlot
				xField={$store.chart === 'speed' ? 'signingTime' : 'pk'}
				yField={$store.chart === 'speed' ? 'verificationTime' : 'sig'}
			/>
		</div>

		{#if !timed}
			<p role="note" class="text-xs text-pqs-steel dark:text-pqs-bluegray">
				Sizes only for now: none of these candidates is in our benchmark harness yet, so the table
				shows them as not yet benchmarked. Timings will follow once they are added.
			</p>
		{/if}

		<CompareTable
			config={SIGNATURES}
			rows={$filteredRows}
			sortCol={$store.sortCol}
			sortDir={$store.sortDir}
			onsort={(sortCol, sortDir) => store.update((f) => ({ ...f, sortCol, sortDir }))}
		/>

		{#if unmapped.length > 0}
			<p class="text-xs text-pqs-steel dark:text-pqs-bluegray">
				No recommended {target}-bit parameter set in our data: {unmapped.join(', ')}.
			</p>
		{/if}

		<p class="text-right">
			<a href={compareHref(base, p.id, 'signature', { target, stage: view })} class="font-heading text-sm font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot">
				Open in the signature comparison →
			</a>
		</p>
	</div>
{/if}
