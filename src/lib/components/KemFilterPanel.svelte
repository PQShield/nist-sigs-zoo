<script lang="ts">
	import type { CommonFilter, KemDataRanges, KemFilterState, KemScheme, ProgrammeGroup } from '$lib/types';
	import { defaultKemFilter } from '$lib/kemData';
	import RangeField from './RangeField.svelte';
	import ProgrammeFilter from './ProgrammeFilter.svelte';
	import LevelFilter from './LevelFilter.svelte';
	import ParamSetFilter from './ParamSetFilter.svelte';
	import StatusFilter from './StatusFilter.svelte';
	import { FILTER_HEADING } from './filterStyles';
	import { rowProgrammeGroups } from '$lib/compareFilters';

	interface Props {
		schemes: KemScheme[];
		categories: string[];
		ranges: KemDataRanges;
		groups: ProgrammeGroup[];
		filter: KemFilterState;
	}
	let { schemes, categories, ranges, groups, filter = $bindable() }: Props = $props();

	const allSchemeNames = $derived(schemes.map((s) => s.scheme));

	// Only schemes in a selected programme are listed (their checked state is kept while hidden).
	const listedSchemes = $derived(
		schemes.filter((s) => rowProgrammeGroups(s).some((g) => filter.programmes.has(g)))
	);
	const schemesByCategory = $derived(
		Object.fromEntries(categories.map((cat) => [cat, listedSchemes.filter((s) => s.category === cat)]))
	);
	const listedCategories = $derived(categories.filter((cat) => schemesByCategory[cat].length > 0));

	function patch(p: Partial<CommonFilter>) {
		filter = { ...filter, ...p };
	}

	function categoryState(cat: string): 'all' | 'some' | 'none' {
		const catSchemes = schemesByCategory[cat];
		const n = catSchemes.filter((s) => filter.schemes.has(s.scheme)).length;
		if (n === catSchemes.length) return 'all';
		if (n === 0) return 'none';
		return 'some';
	}

	function toggleCategory(cat: string) {
		const catSchemes = schemesByCategory[cat].map((s) => s.scheme);
		const next = new Set(filter.schemes);
		if (categoryState(cat) === 'all') catSchemes.forEach((s) => next.delete(s));
		else catSchemes.forEach((s) => next.add(s));
		filter = { ...filter, schemes: next };
	}

	function toggleScheme(scheme: string) {
		const next = new Set(filter.schemes);
		if (next.has(scheme)) next.delete(scheme);
		else next.add(scheme);
		filter = { ...filter, schemes: next };
	}

	function selectAll() {
		filter = { ...filter, schemes: new Set(allSchemeNames) };
	}

	function selectNone() {
		filter = { ...filter, schemes: new Set() };
	}

	function reset() {
		filter = { ...defaultKemFilter(ranges, allSchemeNames, groups), chart: filter.chart };
	}

	function indeterminate(el: HTMLInputElement, value: boolean) {
		el.indeterminate = value;
		return {
			update(v: boolean) {
				el.indeterminate = v;
			}
		};
	}
</script>

<aside class="rounded border border-pqs-ashgray bg-white p-4 text-sm shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
	<h2 class="mb-4 hidden font-heading text-base font-bold text-pqs-midnight dark:text-white lg:block">Filters</h2>

	<ProgrammeFilter {filter} {groups} onchange={patch} />

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<section class="mb-5">
		<div class="mb-2 flex items-center justify-between">
			<h3 class={FILTER_HEADING}>Schemes</h3>
			<span class="space-x-2 font-heading text-xs">
				<button type="button" onclick={selectAll} class="text-pqs-apricot underline hover:no-underline">All</button>
				<button type="button" onclick={selectNone} class="text-pqs-apricot underline hover:no-underline">None</button>
			</span>
		</div>
		{#each listedCategories as cat (cat)}
			<details class="mb-1" open>
				<summary class="flex cursor-pointer list-none items-center gap-1.5 py-0.5">
					<input
						type="checkbox"
						aria-label="All {cat === 'Pre-Quantum' ? 'Pre-quantum' : cat} schemes"
						checked={categoryState(cat) !== 'none'}
						use:indeterminate={categoryState(cat) === 'some'}
						onchange={() => toggleCategory(cat)}
						class="shrink-0 accent-pqs-apricot"
					/>
					<span class="font-heading font-semibold text-pqs-midnight dark:text-pqs-smoke">{cat === 'Pre-Quantum' ? 'Pre-quantum' : cat}</span>
				</summary>
				<div class="ml-5 mt-0.5 space-y-0.5">
					{#each schemesByCategory[cat] as scheme (scheme.scheme)}
						<label class="flex cursor-pointer items-center gap-1.5">
							<input
								type="checkbox"
								checked={filter.schemes.has(scheme.scheme)}
								onchange={() => toggleScheme(scheme.scheme)}
								class="shrink-0 accent-pqs-apricot"
							/>
							<span class="text-pqs-steel dark:text-pqs-bluegray">{scheme.scheme}</span>
						</label>
					{/each}
				</div>
			</details>
		{/each}
	</section>

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<LevelFilter {filter} onchange={patch} />
	<ParamSetFilter {filter} onchange={patch} />

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<section class="mb-5">
		<h3 class={FILTER_HEADING}>Sizes (bytes)</h3>
		<div class="space-y-2">
			<RangeField label="Min pk" value={filter.minPk} onchange={(v) => (filter = { ...filter, minPk: v })} />
			<RangeField label="Max pk" value={filter.maxPk} onchange={(v) => (filter = { ...filter, maxPk: v })} />
			<RangeField label="Min ct" value={filter.minCt} onchange={(v) => (filter = { ...filter, minCt: v })} />
			<RangeField label="Max ct" value={filter.maxCt} onchange={(v) => (filter = { ...filter, maxCt: v })} />
			<RangeField label="Min pk+ct" value={filter.minPkPlusCt} onchange={(v) => (filter = { ...filter, minPkPlusCt: v })} />
			<RangeField label="Max pk+ct" value={filter.maxPkPlusCt} onchange={(v) => (filter = { ...filter, maxPkPlusCt: v })} />
		</div>
	</section>

	{#if ranges.keygenUs || ranges.encapsUs || ranges.decapsUs}
		<section class="mb-5">
			<h3 class={FILTER_HEADING}>Performance (µs)</h3>
			<div class="space-y-2">
				{#if ranges.keygenUs}
					<RangeField label="Min keygen" value={filter.minKeygenUs} onchange={(v) => (filter = { ...filter, minKeygenUs: v })} />
					<RangeField label="Max keygen" value={filter.maxKeygenUs} onchange={(v) => (filter = { ...filter, maxKeygenUs: v })} />
				{/if}
				{#if ranges.encapsUs}
					<RangeField label="Min encaps" value={filter.minEncapsUs} onchange={(v) => (filter = { ...filter, minEncapsUs: v })} />
					<RangeField label="Max encaps" value={filter.maxEncapsUs} onchange={(v) => (filter = { ...filter, maxEncapsUs: v })} />
				{/if}
				{#if ranges.decapsUs}
					<RangeField label="Min decaps" value={filter.minDecapsUs} onchange={(v) => (filter = { ...filter, minDecapsUs: v })} />
					<RangeField label="Max decaps" value={filter.maxDecapsUs} onchange={(v) => (filter = { ...filter, maxDecapsUs: v })} />
				{/if}
			</div>
		</section>
	{/if}

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<StatusFilter {filter} onchange={patch} />

	<button
		type="button"
		onclick={reset}
		class="h-10 w-full rounded border border-pqs-bluegray font-heading font-semibold text-pqs-steel hover:bg-pqs-smoke dark:border-pqs-steel dark:text-pqs-apricot dark:hover:bg-pqs-steel/30"
	>Reset filters</button>
</aside>
