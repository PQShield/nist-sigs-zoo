<script lang="ts">
	import type { DataRanges, FilterState, NistLevel, Scheme } from '$lib/types';
	import { getFilterStore } from '$lib/filterStore';
	import { ALL_LEVELS } from '$lib/constants';
	import RangeField from './RangeField.svelte';

	interface Props {
		schemes: Scheme[];
		categories: string[];
		ranges: DataRanges;
	}
	let { schemes, categories, ranges }: Props = $props();

	const { store, defaults } = getFilterStore();

	const schemesByCategory = $derived(
		Object.fromEntries(
			categories.map((cat) => [cat, schemes.filter((s) => s.category === cat)])
		)
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
		const state = categoryState(cat);
		const next = new Set($store.schemes);
		if (state === 'all') {
			catSchemes.forEach((s) => next.delete(s));
		} else {
			catSchemes.forEach((s) => next.add(s));
		}
		store.update((f) => ({ ...f, schemes: next }));
	}

	function toggleScheme(scheme: string) {
		const next = new Set($store.schemes);
		if (next.has(scheme)) next.delete(scheme);
		else next.add(scheme);
		store.update((f) => ({ ...f, schemes: next }));
	}

	function toggleLevel(level: NistLevel) {
		const next = new Set($store.levels);
		if (next.has(level)) next.delete(level);
		else next.add(level);
		store.update((f) => ({ ...f, levels: next }));
	}

	function selectAll() {
		store.update((f) => ({ ...f, schemes: new Set(defaults.schemes) }));
	}

	function selectNone() {
		store.update((f) => ({ ...f, schemes: new Set() }));
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
	<h2 class="mb-4 font-heading text-base font-bold text-pqs-midnight dark:text-white">Filters</h2>

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
						checked={$store.levels.has(level)}
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
	<section class="mb-5">
		<h3 class="mb-2 font-heading text-xs font-semibold uppercase tracking-wider text-pqs-steel dark:text-pqs-apricot">
			Key / Sig Sizes (bytes)
		</h3>
		<div class="space-y-2">
			<RangeField label="Min pk" value={$store.minPk} onchange={(v) => store.update((f) => ({ ...f, minPk: v }))} />
			<RangeField label="Max pk" value={$store.maxPk} onchange={(v) => store.update((f) => ({ ...f, maxPk: v }))} />
			<RangeField label="Min sig" value={$store.minSig} onchange={(v) => store.update((f) => ({ ...f, minSig: v }))} />
			<RangeField label="Max sig" value={$store.maxSig} onchange={(v) => store.update((f) => ({ ...f, maxSig: v }))} />
			<RangeField label="Min pk+sig" value={$store.minPkPlusSig} onchange={(v) => store.update((f) => ({ ...f, minPkPlusSig: v }))} />
			<RangeField label="Max pk+sig" value={$store.maxPkPlusSig} onchange={(v) => store.update((f) => ({ ...f, maxPkPlusSig: v }))} />
		</div>
	</section>

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<!-- Performance filters -->
	<section class="mb-5">
		<h3 class="mb-2 font-heading text-xs font-semibold uppercase tracking-wider text-pqs-steel dark:text-pqs-apricot">
			Performance (cycles)
		</h3>
		<div class="space-y-2">
			<RangeField label="Min signing" value={$store.minSigningCycles} onchange={(v) => store.update((f) => ({ ...f, minSigningCycles: v }))} />
			<RangeField label="Max signing" value={$store.maxSigningCycles} onchange={(v) => store.update((f) => ({ ...f, maxSigningCycles: v }))} />
			<RangeField label="Min verification" value={$store.minVerificationCycles} onchange={(v) => store.update((f) => ({ ...f, minVerificationCycles: v }))} />
			<RangeField label="Max verification" value={$store.maxVerificationCycles} onchange={(v) => store.update((f) => ({ ...f, maxVerificationCycles: v }))} />
		</div>
	</section>

	{#if ranges.signingUs || ranges.verificationUs}
		<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

		<section>
			<h3 class="mb-2 font-heading text-xs font-semibold uppercase tracking-wider text-pqs-steel dark:text-pqs-apricot">
				Performance (µs)
			</h3>
			<div class="space-y-2">
				{#if ranges.signingUs}
					<RangeField label="Min signing" value={$store.minSigningUs} onchange={(v) => store.update((f) => ({ ...f, minSigningUs: v }))} />
					<RangeField label="Max signing" value={$store.maxSigningUs} onchange={(v) => store.update((f) => ({ ...f, maxSigningUs: v }))} />
				{/if}
				{#if ranges.verificationUs}
					<RangeField label="Min verification" value={$store.minVerificationUs} onchange={(v) => store.update((f) => ({ ...f, minVerificationUs: v }))} />
					<RangeField label="Max verification" value={$store.maxVerificationUs} onchange={(v) => store.update((f) => ({ ...f, maxVerificationUs: v }))} />
				{/if}
			</div>
		</section>
	{/if}
</aside>
