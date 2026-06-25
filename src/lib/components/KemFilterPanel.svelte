<script lang="ts">
	import type { Kem, NistLevel } from '$lib/types';
	import { getKemStore } from '$lib/kemStore';

	interface Props {
		kems: Kem[];
		categories: string[];
	}
	let { kems, categories }: Props = $props();

	const { store, defaults } = getKemStore();

	const kemsByCategory = $derived(
		Object.fromEntries(categories.map((cat) => [cat, kems.filter((s) => s.category === cat)]))
	);

	const ALL_LEVELS: NistLevel[] = ['Pre-Quantum', 1, 2, 3, 4, 5];

	function categoryState(cat: string): 'all' | 'some' | 'none' {
		const catKems = kemsByCategory[cat];
		const n = catKems.filter((s) => $store.schemes.has(s.scheme)).length;
		if (n === catKems.length) return 'all';
		if (n === 0) return 'none';
		return 'some';
	}

	function toggleCategory(cat: string) {
		const catKems = kemsByCategory[cat].map((s) => s.scheme);
		const state = categoryState(cat);
		const next = new Set($store.schemes);
		if (state === 'all') {
			catKems.forEach((s) => next.delete(s));
		} else {
			catKems.forEach((s) => next.add(s));
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
					{#each kemsByCategory[cat] as kem}
						<label class="flex cursor-pointer items-center gap-1.5">
							<input
								type="checkbox"
								checked={$store.schemes.has(kem.scheme)}
								onchange={() => toggleScheme(kem.scheme)}
								class="shrink-0 accent-pqs-apricot"
							/>
							<span class="text-pqs-steel dark:text-pqs-bluegray">{kem.scheme}</span>
						</label>
					{/each}
				</div>
			</details>
		{/each}
	</section>

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel">

	<section>
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
</aside>
