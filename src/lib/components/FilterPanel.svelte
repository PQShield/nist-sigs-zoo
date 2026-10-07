<script lang="ts">
	import type { Snippet } from 'svelte';
	import type { CommonFilter, DataRanges, ProgrammeGroup, Scheme } from '$lib/types';
	import { getFilterStore } from '$lib/filterStore';
	import RangeField from './RangeField.svelte';
	import ProgrammeFilter from './ProgrammeFilter.svelte';
	import LevelFilter from './LevelFilter.svelte';
	import ParamSetFilter from './ParamSetFilter.svelte';
	import StatusFilter from './StatusFilter.svelte';
	import { FILTER_HEADING } from './filterStyles';

	interface Props {
		schemes: Scheme[];
		categories: string[];
		ranges: DataRanges;
		groups: ProgrammeGroup[];
		/** Rendered under "NIST on-ramp" (the on-ramp round select). */
		onramp?: Snippet<[boolean]>;
	}
	let { schemes, categories, ranges, groups, onramp }: Props = $props();

	const { store } = getFilterStore();

	function patch(p: Partial<CommonFilter>) {
		store.update((f) => ({ ...f, ...p }));
	}

	const schemesByCategory = $derived(
		Object.fromEntries(categories.map((cat) => [cat, schemes.filter((s) => s.category === cat)]))
	);

	function categoryState(cat: string): 'all' | 'some' | 'none' {
		const catSchemes = schemesByCategory[cat];
		const n = catSchemes.filter((s) => $store.schemes.has(s.scheme)).length;
		if (n === catSchemes.length) return 'all';
		if (n === 0) return 'none';
		return 'some';
	}

	function toggleCategory(cat: string) {
		const catSchemes = schemesByCategory[cat].map((s) => s.scheme);
		const next = new Set($store.schemes);
		if (categoryState(cat) === 'all') catSchemes.forEach((s) => next.delete(s));
		else catSchemes.forEach((s) => next.add(s));
		store.update((f) => ({ ...f, schemes: next }));
	}

	function toggleScheme(scheme: string) {
		const next = new Set($store.schemes);
		if (next.has(scheme)) next.delete(scheme);
		else next.add(scheme);
		store.update((f) => ({ ...f, schemes: next }));
	}

	// Defaults are read at call time: they change when the round changes.
	function selectAll() {
		store.update((f) => ({ ...f, schemes: new Set(getFilterStore().defaults.schemes) }));
	}

	function selectNone() {
		store.update((f) => ({ ...f, schemes: new Set() }));
	}

	function reset() {
		store.update((f) => ({ ...getFilterStore().defaults, sortCol: f.sortCol, sortDir: f.sortDir, chart: f.chart }));
	}

	function indeterminate(el: HTMLInputElement, value: boolean) {
		el.indeterminate = value;
		return {
			update(v: boolean) {
				el.indeterminate = v;
			},
		};
	}
</script>

<aside class="rounded border border-pqs-ashgray bg-white p-4 text-sm shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
	<h2 class="mb-4 hidden font-heading text-base font-bold text-pqs-midnight dark:text-white lg:block">Filters</h2>

	<ProgrammeFilter filter={$store} {groups} onchange={patch} {onramp} />

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<section class="mb-5">
		<div class="mb-2 flex items-center justify-between">
			<h3 class={FILTER_HEADING}>Schemes</h3>
			<span class="space-x-2 font-heading text-xs">
				<button type="button" onclick={selectAll} class="text-pqs-apricot underline hover:no-underline">All</button>
				<button type="button" onclick={selectNone} class="text-pqs-apricot underline hover:no-underline">None</button>
			</span>
		</div>
		{#each categories as cat (cat)}
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
								checked={$store.schemes.has(scheme.scheme)}
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

	<LevelFilter filter={$store} onchange={patch} />
	<ParamSetFilter filter={$store} onchange={patch} />

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<section class="mb-5">
		<h3 class={FILTER_HEADING}>Sizes (bytes)</h3>
		<div class="space-y-2">
			<RangeField label="Min pk" value={$store.minPk} onchange={(v) => store.update((f) => ({ ...f, minPk: v }))} />
			<RangeField label="Max pk" value={$store.maxPk} onchange={(v) => store.update((f) => ({ ...f, maxPk: v }))} />
			<RangeField label="Min sig" value={$store.minSig} onchange={(v) => store.update((f) => ({ ...f, minSig: v }))} />
			<RangeField label="Max sig" value={$store.maxSig} onchange={(v) => store.update((f) => ({ ...f, maxSig: v }))} />
			<RangeField label="Min pk+sig" value={$store.minPkPlusSig} onchange={(v) => store.update((f) => ({ ...f, minPkPlusSig: v }))} />
			<RangeField label="Max pk+sig" value={$store.maxPkPlusSig} onchange={(v) => store.update((f) => ({ ...f, maxPkPlusSig: v }))} />
		</div>
	</section>

	<section class="mb-5">
		<h3 class={FILTER_HEADING}>Performance (cycles)</h3>
		<div class="space-y-2">
			<RangeField label="Min signing" value={$store.minSigningCycles} onchange={(v) => store.update((f) => ({ ...f, minSigningCycles: v }))} />
			<RangeField label="Max signing" value={$store.maxSigningCycles} onchange={(v) => store.update((f) => ({ ...f, maxSigningCycles: v }))} />
			<RangeField label="Min verification" value={$store.minVerificationCycles} onchange={(v) => store.update((f) => ({ ...f, minVerificationCycles: v }))} />
			<RangeField label="Max verification" value={$store.maxVerificationCycles} onchange={(v) => store.update((f) => ({ ...f, maxVerificationCycles: v }))} />
		</div>
	</section>

	{#if ranges.signingUs || ranges.verificationUs}
		<section class="mb-5">
			<h3 class={FILTER_HEADING}>Performance (µs)</h3>
			<div class="space-y-2">
				{#if ranges.signingUs}
					<RangeField label="Min signing µs" value={$store.minSigningUs} onchange={(v) => store.update((f) => ({ ...f, minSigningUs: v }))} />
					<RangeField label="Max signing µs" value={$store.maxSigningUs} onchange={(v) => store.update((f) => ({ ...f, maxSigningUs: v }))} />
				{/if}
				{#if ranges.verificationUs}
					<RangeField label="Min verification µs" value={$store.minVerificationUs} onchange={(v) => store.update((f) => ({ ...f, minVerificationUs: v }))} />
					<RangeField label="Max verification µs" value={$store.maxVerificationUs} onchange={(v) => store.update((f) => ({ ...f, maxVerificationUs: v }))} />
				{/if}
			</div>
		</section>
	{/if}

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<StatusFilter filter={$store} onchange={patch} />

	<button
		type="button"
		onclick={reset}
		class="h-10 w-full rounded border border-pqs-bluegray font-heading font-semibold text-pqs-steel hover:bg-pqs-smoke dark:border-pqs-steel dark:text-pqs-apricot dark:hover:bg-pqs-steel/30"
	>Reset filters</button>
</aside>
