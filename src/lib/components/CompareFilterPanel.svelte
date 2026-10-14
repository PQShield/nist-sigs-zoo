<script lang="ts" generics="R extends CompareRow, G, F extends CompareFilter">
	import type { Snippet } from 'svelte';
	import type { CommonFilter, CompareFilter, ProgrammeGroup, ProgrammeMembership } from '$lib/types';
	import type { BoundSpec, CompareRow, FilterSpec } from '$lib/compare';
	import RangeField from './RangeField.svelte';
	import ProgrammeFilter from './ProgrammeFilter.svelte';
	import LevelFilter from './LevelFilter.svelte';
	import ParamSetFilter from './ParamSetFilter.svelte';
	import StatusFilter from './StatusFilter.svelte';
	import { FILTER_HEADING } from './filterStyles';
	import { rowProgrammeGroups } from '$lib/compareFilters';

	interface Props {
		spec: FilterSpec<R, G, F>;
		schemes: { scheme: string; category: string; programmes: ProgrammeMembership[]; classical: boolean }[];
		categories: string[];
		ranges: G;
		groups: ProgrammeGroup[];
		defaults: F;
		filter: F;
		/** Rendered under "NIST on-ramp" (the on-ramp round select). */
		onramp?: Snippet<[boolean]>;
	}
	let { spec, schemes, categories, ranges, groups, defaults, filter = $bindable(), onramp }: Props = $props();

	// Bound sections in spec order; fields without data (null range) are left out.
	const sections = $derived.by(() => {
		const out: { title: string; bounds: BoundSpec<R, G>[] }[] = [];
		for (const b of spec.bounds) {
			if (b.range(ranges) === null) continue;
			const last = out.at(-1);
			if (last?.title === b.section) last.bounds.push(b);
			else out.push({ title: b.section, bounds: [b] });
		}
		return out;
	});

	const bound = (key: string): number => (filter as unknown as Record<string, number>)[key];

	function setBound(key: string, v: number) {
		filter = { ...filter, [key]: v };
	}

	// Only schemes in a selected programme are listed (their checked state is kept while hidden).
	const listedSchemes = $derived(schemes.filter((s) => rowProgrammeGroups(s).some((g) => filter.programmes.has(g))));
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
		filter = { ...filter, schemes: new Set(defaults.schemes) };
	}

	function selectNone() {
		filter = { ...filter, schemes: new Set() };
	}

	function reset() {
		filter = { ...defaults, sortCol: filter.sortCol, sortDir: filter.sortDir, chart: filter.chart };
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

<aside
	class="rounded border border-pqs-ashgray bg-white p-4 text-sm shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid"
>
	<h2 class="mb-4 hidden font-heading text-base font-bold text-pqs-midnight lg:block dark:text-white">Filters</h2>

	<ProgrammeFilter {filter} {groups} onchange={patch} {onramp} />

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
					<span class="font-heading font-semibold text-pqs-midnight dark:text-pqs-smoke"
						>{cat === 'Pre-Quantum' ? 'Pre-quantum' : cat}</span
					>
				</summary>
				<div class="mt-0.5 ml-5 space-y-0.5">
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

	{#each sections as section (section.title)}
		<section class="mb-5">
			<h3 class={FILTER_HEADING}>{section.title}</h3>
			<div class="space-y-2">
				{#each section.bounds as b (b.field)}
					<RangeField
						label="Min {b.label}"
						value={bound(`min${b.field}`)}
						onchange={(v) => setBound(`min${b.field}`, v)}
					/>
					<RangeField
						label="Max {b.label}"
						value={bound(`max${b.field}`)}
						onchange={(v) => setBound(`max${b.field}`, v)}
					/>
				{/each}
			</div>
		</section>
	{/each}

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<StatusFilter {filter} onchange={patch} />

	<button
		type="button"
		onclick={reset}
		class="h-10 w-full rounded border border-pqs-bluegray font-heading font-semibold text-pqs-steel hover:bg-pqs-smoke dark:border-pqs-steel dark:text-pqs-apricot dark:hover:bg-pqs-steel/30"
		>Reset filters</button
	>
</aside>
