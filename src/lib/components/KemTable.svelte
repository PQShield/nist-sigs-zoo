<script lang="ts">
	import type { KemSortableColumn } from '$lib/types';
	import { getKemStore } from '$lib/kemStore';
	import SecurityBadge from './SecurityBadge.svelte';

	const { store, filteredRows } = getKemStore();

	function fmt(n: number) {
		return n.toLocaleString();
	}

	function fmtTime(us: number): string {
		if (us >= 1_000_000) return (us / 1_000_000).toFixed(2) + ' s';
		if (us >= 1_000) return (us / 1_000).toFixed(2) + ' ms';
		return us.toFixed(1) + ' µs';
	}

	const COLUMNS: { key: KemSortableColumn; label: string; numeric?: boolean }[] = [
		{ key: 'scheme', label: 'Scheme' },
		{ key: 'category', label: 'Category' },
		{ key: 'status', label: 'Status' },
		{ key: 'parameterset', label: 'Parameter Set' },
		{ key: 'level', label: 'Level', numeric: true },
		{ key: 'pk', label: 'pk (B)', numeric: true },
		{ key: 'ct', label: 'ct (B)', numeric: true },
		{ key: 'pkPlusCt', label: 'pk+ct (B)', numeric: true },
		{ key: 'sk', label: 'sk (B)', numeric: true },
		{ key: 'keygenUs', label: 'Keygen', numeric: true },
		{ key: 'encapsUs', label: 'Encaps', numeric: true },
		{ key: 'decapsUs', label: 'Decaps', numeric: true },
	];

	function setSort(col: KemSortableColumn) {
		store.update((f) => ({
			...f,
			sortCol: col,
			sortDir: f.sortCol === col && f.sortDir === 'asc' ? 'desc' : 'asc',
		}));
	}

	function sortIndicator(col: KemSortableColumn): string {
		if ($store.sortCol !== col) return '';
		return $store.sortDir === 'asc' ? ' ▲' : ' ▼';
	}
</script>

{#snippet timing(us: number | null, cycles: number)}
	<td class="px-3 py-1.5 text-right tabular-nums">
		{#if us != null}
			{fmtTime(us)}
		{:else if cycles > 0}
			<span class="underline decoration-wavy decoration-pqs-scarlet">
				{fmtTime(cycles / 2500)}
			</span>
		{:else}
			<span class="text-pqs-bluegray">—</span>
		{/if}
	</td>
{/snippet}

<div class="overflow-x-auto rounded border border-pqs-ashgray shadow-sm dark:border-pqs-steel">
	<table class="min-w-full text-sm">
		<thead class="sticky top-0 z-10">
			<tr class="bg-pqs-steel font-heading text-xs text-white">
				{#each COLUMNS as col}
					<th
						class="cursor-pointer select-none whitespace-nowrap px-3 py-2.5 text-left font-semibold hover:bg-pqs-steel-light {col.numeric ? 'text-right' : ''}"
						onclick={() => setSort(col.key)}
					>
						{col.label}{sortIndicator(col.key)}
					</th>
				{/each}
				<th class="px-3 py-2.5 text-left text-xs font-semibold">Assumption</th>
			</tr>
		</thead>
		<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
			{#each $filteredRows as row (row.scheme + row.parameterset)}
				<tr class="hover:bg-pqs-smoke dark:hover:bg-pqs-steel/30">
					<td class="whitespace-nowrap px-3 py-1.5">
						<a href={row.website} class="font-heading font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot dark:hover:text-pqs-apricot-light">
							{row.scheme}
						</a>
						<SecurityBadge
							broken={row.broken}
							warning={row.warning}
							info={row.info}
							classical={row.classical}
						/>
					</td>
					<td class="whitespace-nowrap px-3 py-1.5 text-pqs-steel dark:text-pqs-bluegray">
						{row.category}
					</td>
					<td class="whitespace-nowrap px-3 py-1.5">
						{#if row.status === 'FIPS'}
							<span class="rounded bg-pqs-apricot/20 px-1.5 py-0.5 text-xs font-semibold text-pqs-apricot">FIPS</span>
						{:else if row.status === 'To be standardized'}
							<span class="rounded bg-pqs-steel/10 px-1.5 py-0.5 text-xs font-semibold text-pqs-steel dark:text-pqs-bluegray">Std pending</span>
						{:else if row.status === 'Classic cryptography'}
							<span class="text-pqs-steel/70 dark:text-pqs-bluegray/70">Classic</span>
						{:else}
							<span class="text-pqs-steel/70 dark:text-pqs-bluegray/70">{row.status}</span>
						{/if}
					</td>
					<td class="whitespace-nowrap px-3 py-1.5 font-mono text-xs">{row.parameterset}</td>
					<td class="px-3 py-1.5 text-right tabular-nums text-pqs-steel dark:text-pqs-bluegray">
						{row.level === 'Pre-Quantum' ? 'N/A' : row.level}
					</td>
					<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pk)}</td>
					<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.ct)}</td>
					<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pkPlusCt)}</td>
					<td class="px-3 py-1.5 text-right tabular-nums">
						{#if row.sk != null}
							{fmt(row.sk)}
						{:else}
							<span class="text-pqs-bluegray">—</span>
						{/if}
					</td>
					{@render timing(row.keygenUs, row.keygenCycles)}
					{@render timing(row.encapsUs, row.encapsCycles)}
					{@render timing(row.decapsUs, row.decapsCycles)}
					<td class="px-3 py-1.5 text-pqs-steel/80 dark:text-pqs-bluegray">{row.assumption}</td>
				</tr>
			{/each}
			{#if $filteredRows.length === 0}
				<tr>
					<td colspan="13" class="px-3 py-10 text-center text-pqs-bluegray">
						No parameter sets match the current filters.
					</td>
				</tr>
			{/if}
		</tbody>
	</table>
	<div class="border-t border-pqs-ashgray bg-pqs-smoke px-3 py-1.5 font-heading text-xs text-pqs-steel dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-bluegray">
		{$filteredRows.length} parameter set{$filteredRows.length === 1 ? '' : 's'}
	</div>
	<div class="border-t border-pqs-ashgray bg-pqs-smoke px-3 py-2 text-xs text-pqs-steel/70 dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-bluegray/70">
		<span class="font-semibold text-pqs-steel dark:text-pqs-bluegray">Legend:</span>
		<span class="ml-2">💣 pre-quantum (classical security only)</span>
		<span class="ml-2">·</span>
		<span class="ml-2">🧨 broken</span>
		<span class="ml-2">·</span>
		<span class="ml-2">⚠️ security warning</span>
		<span class="ml-2">·</span>
		<span class="ml-2">ℹ️ note</span>
		<span class="ml-2">·</span>
		<span class="ml-2"><span class="underline decoration-wavy decoration-pqs-scarlet">value</span> estimated from cycle counts</span>
		<span class="ml-2">· tap icons for details</span>
	</div>
</div>
