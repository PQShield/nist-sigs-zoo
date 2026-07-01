<script lang="ts">
	import type { KemDataRanges, KemFilterState, KemScheme, NistLevel } from '$lib/types';
	import { defaultKemFilter } from '$lib/kemData';
	import RangeField from './RangeField.svelte';

	interface Props {
		schemes: KemScheme[];
		categories: string[];
		ranges: KemDataRanges;
		filter: KemFilterState;
	}
	let { schemes, categories, ranges, filter = $bindable() }: Props = $props();

	const allSchemeNames = $derived(schemes.map((s) => s.scheme));

	const schemesByCategory = $derived(
		Object.fromEntries(categories.map((cat) => [cat, schemes.filter((s) => s.category === cat)]))
	);

	const ALL_LEVELS: NistLevel[] = ['Pre-Quantum', 1, 2, 3, 4, 5];

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
		if (categoryState(cat) === 'all') {
			catSchemes.forEach((s) => next.delete(s));
		} else {
			catSchemes.forEach((s) => next.add(s));
		}
		filter = { ...filter, schemes: next };
	}

	function toggleScheme(scheme: string) {
		const next = new Set(filter.schemes);
		if (next.has(scheme)) next.delete(scheme);
		else next.add(scheme);
		filter = { ...filter, schemes: next };
	}

	function toggleLevel(level: NistLevel) {
		const next = new Set(filter.levels);
		if (next.has(level)) next.delete(level);
		else next.add(level);
		filter = { ...filter, levels: next };
	}

	function selectAll() {
		filter = { ...filter, schemes: new Set(allSchemeNames) };
	}

	function selectNone() {
		filter = { ...filter, schemes: new Set() };
	}

	function reset() {
		filter = defaultKemFilter(ranges, allSchemeNames);
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
	<div class="mb-4 flex items-center justify-between">
		<h2 class="font-heading text-base font-bold text-pqs-midnight dark:text-white">Filters</h2>
		<button onclick={reset} class="font-heading text-xs text-pqs-apricot underline hover:no-underline">Reset</button>
	</div>

	<!-- Scheme filters -->
	<section class="mb-5">
		<div class="mb-2 flex items-center justify-between">
			<h3 class="font-heading text-xs font-semibold uppercase tracking-wider text-pqs-steel dark:text-pqs-apricot">
				Schemes
			</h3>
			<span class="space-x-2 font-heading text-xs">
				<button onclick={selectAll} class="text-pqs-apricot underline hover:no-underline">All</button>
				<button onclick={selectNone} class="text-pqs-apricot underline hover:no-underline">None</button>
			</span>
		</div>
		{#each categories as cat}
			<details class="mb-1" open>
				<summary class="flex cursor-pointer list-none items-center gap-1.5 py-0.5">
					<input
						type="checkbox"
						checked={categoryState(cat) !== 'none'}
						use:indeterminate={categoryState(cat) === 'some'}
						onchange={() => toggleCategory(cat)}
						class="shrink-0 accent-pqs-apricot"
					/>
					<span class="font-heading font-semibold text-pqs-midnight dark:text-pqs-smoke">{cat}</span>
				</summary>
				<div class="ml-5 mt-0.5 space-y-0.5">
					{#each schemesByCategory[cat] as scheme}
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

	<!-- Level filters -->
	<section class="mb-5">
		<h3 class="mb-2 font-heading text-xs font-semibold uppercase tracking-wider text-pqs-steel dark:text-pqs-apricot">
			NIST Security Level
		</h3>
		<div class="space-y-0.5">
			{#each ALL_LEVELS as level}
				<label class="flex cursor-pointer items-center gap-1.5">
					<input
						type="checkbox"
						checked={filter.levels.has(level)}
						onchange={() => toggleLevel(level)}
						class="accent-pqs-apricot"
					/>
					<span class="text-pqs-steel dark:text-pqs-bluegray">
						{level === 'Pre-Quantum' ? 'N/A' : `Level ${level}`}
					</span>
				</label>
			{/each}
		</div>
	</section>

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<!-- Size filters -->
	<section>
		<h3 class="mb-2 font-heading text-xs font-semibold uppercase tracking-wider text-pqs-steel dark:text-pqs-apricot">
			Sizes (bytes)
		</h3>
		<div class="space-y-2">
			<RangeField label="Min pk" value={filter.minPk} onchange={(v) => (filter = { ...filter, minPk: v })} />
			<RangeField label="Max pk" value={filter.maxPk} onchange={(v) => (filter = { ...filter, maxPk: v })} />
			<RangeField label="Min ct" value={filter.minCt} onchange={(v) => (filter = { ...filter, minCt: v })} />
			<RangeField label="Max ct" value={filter.maxCt} onchange={(v) => (filter = { ...filter, maxCt: v })} />
			<RangeField label="Min pk+ct" value={filter.minPkPlusCt} onchange={(v) => (filter = { ...filter, minPkPlusCt: v })} />
			<RangeField label="Max pk+ct" value={filter.maxPkPlusCt} onchange={(v) => (filter = { ...filter, maxPkPlusCt: v })} />
		</div>
	</section>
</aside>
