<script lang="ts" generics="R extends CompareRow, G, F extends CompareFilter, A extends string">
	import { untrack } from 'svelte';
	import { base } from '$app/paths';
	import { presentProgrammeGroups } from '$lib/compareFilters';
	import {
		anyTimings,
		defaultCompareFilter,
		filterCompareRows,
		type CompareConfig,
		type CompareRow
	} from '$lib/compare';
	import {
		compareHref,
		CURRENT_VIEW,
		focusTarget,
		ngccTargets,
		unmappedAtTarget,
		type StageView
	} from '$lib/programmePage';
	import type { CompareFilter, NgccLevel, ProgrammeYaml } from '$lib/types';
	import ChartModeToggle from '../ChartModeToggle.svelte';
	import CompareChart from '../CompareChart.svelte';
	import CompareTable from '../CompareTable.svelte';
	import SegmentedToggle from '../SegmentedToggle.svelte';

	interface Props {
		programme: ProgrammeYaml;
		config: CompareConfig<R, G, F, A>;
		/** The programme's rows for a stage view (CURRENT_VIEW: every member's newest version). */
		load: (view: string) => { parameterSets: R[]; ranges: G };
		/** Stage toggle; empty for none. */
		views?: StageView[];
	}
	let { programme, config, load, views = [] }: Props = $props();

	const p = untrack(() => programme);
	const cfg = untrack(() => config);
	const loadView = untrack(() => load);
	const initial = loadView(CURRENT_VIEW);
	const firstTarget = ngccTargets(initial.parameterSets)[0] ?? null;

	const defaultsFor = (r: { parameterSets: R[]; ranges: G }) =>
		defaultCompareFilter(
			cfg.filter,
			r.ranges,
			r.parameterSets.map((x) => x.scheme),
			presentProgrammeGroups(r.parameterSets)
		);

	let view = $state(CURRENT_VIEW);
	let rows = $state.raw(initial.parameterSets);
	let target = $state<NgccLevel | null>(firstTarget);
	let filter = $state(focusTarget(defaultsFor(initial), firstTarget));

	const filtered = $derived(filterCompareRows(cfg.filter, rows, filter));
	const chartView = $derived(cfg.chart.views[filter.chart]);

	const targets = $derived(ngccTargets(rows));
	const timed = $derived(anyTimings(cfg.chart, rows));
	const unmapped = $derived(target === null ? [] : unmappedAtTarget(rows, target));

	function chooseView(next: string) {
		view = next;
		const r = loadView(next);
		rows = r.parameterSets;
		target = ngccTargets(r.parameterSets)[0] ?? null;
		// A stage change intentionally resets the other filters.
		filter = focusTarget(defaultsFor(r), target);
	}

	function chooseTarget(next: NgccLevel) {
		target = next;
		filter = focusTarget(filter, next);
	}
</script>

{#if initial.parameterSets.length === 0}
	<p
		class="rounded border border-dashed border-pqs-bluegray px-4 py-6 text-sm text-pqs-steel dark:border-pqs-steel dark:text-pqs-bluegray"
	>
		No {cfg.label.many} from this programme are in our data yet.
	</p>
{:else}
	<div class="space-y-4">
		{#if views.length > 0 || targets.length > 1}
			<div class="flex flex-wrap items-center gap-3">
				{#if views.length > 0}
					<SegmentedToggle
						label="Stage"
						options={views.map((v) => ({ value: v.id, label: v.label }))}
						value={view}
						onchange={chooseView}
					/>
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

		<div
			class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid"
		>
			<div class="mb-3 flex flex-wrap items-center justify-between gap-3">
				<h3 class="font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					{chartView.title}
					<span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h3>
				{#if timed}
					<ChartModeToggle mode={filter.chart} onchange={(chart) => (filter = { ...filter, chart })} />
				{/if}
			</div>
			<CompareChart config={cfg} rows={filtered} xField={chartView.x} yField={chartView.y} />
		</div>

		{#if !timed}
			<p role="note" class="text-xs text-pqs-steel dark:text-pqs-bluegray">
				Sizes only for now: none of these candidates is in our benchmark harness yet, so the table shows them as not yet
				benchmarked. Timings will follow once they are added.
			</p>
		{/if}

		<CompareTable
			config={cfg}
			rows={filtered}
			sortCol={filter.sortCol}
			sortDir={filter.sortDir}
			onsort={(sortCol, sortDir) => (filter = { ...filter, sortCol, sortDir })}
		/>

		{#if unmapped.length > 0}
			<p class="text-xs text-pqs-steel dark:text-pqs-bluegray">
				No recommended {target}-bit parameter set in our data: {unmapped.join(', ')}.
			</p>
		{/if}

		<p class="text-right">
			<a
				href={compareHref(base, p.id, cfg.kind, { target, stage: view })}
				class="font-heading text-sm font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot"
			>
				Open in the {cfg.label.one} comparison →
			</a>
		</p>
	</div>
{/if}
