<script lang="ts">
	import type { KemParameterSet, KemSortableColumn } from '$lib/types';
	import SecurityBadge from './SecurityBadge.svelte';

	let { rows }: { rows: KemParameterSet[] } = $props();

	let sortCol = $state<KemSortableColumn>('pkPlusCt');
	let sortDir = $state<'asc' | 'desc'>('asc');

	function fmt(n: number) {
		return n.toLocaleString();
	}

	function fmtCycles(n: number): string {
		if (n <= 0) return '—';
		if (n >= 1_000_000_000) return (n / 1_000_000_000).toFixed(2) + 'G';
		if (n >= 1_000_000) return (n / 1_000_000).toFixed(1) + 'M';
		if (n >= 1_000) return (n / 1_000).toFixed(0) + 'K';
		return String(n);
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
		{ key: 'keygenUs', label: 'Keygen', numeric: true },
		{ key: 'encapsUs', label: 'Encaps', numeric: true },
		{ key: 'decapsUs', label: 'Decaps', numeric: true }
	];

	const levelOrder: Record<string, number> = {
		'Pre-Quantum': 0,
		1: 1,
		2: 2,
		3: 3,
		4: 4,
		5: 5
	};

	function compare(a: KemParameterSet, b: KemParameterSet, col: KemSortableColumn): number {
		switch (col) {
			case 'level':
				return (levelOrder[String(a.level)] ?? 99) - (levelOrder[String(b.level)] ?? 99);
			case 'pk':
				return a.pk - b.pk;
			case 'ct':
				return a.ct - b.ct;
			case 'pkPlusCt':
				return a.pkPlusCt - b.pkPlusCt;
			case 'keygenUs':
				return (a.keygenUs ?? a.keygenCycles / 2500) - (b.keygenUs ?? b.keygenCycles / 2500);
			case 'encapsUs':
				return (a.encapsUs ?? a.encapsCycles / 2500) - (b.encapsUs ?? b.encapsCycles / 2500);
			case 'decapsUs':
				return (a.decapsUs ?? a.decapsCycles / 2500) - (b.decapsUs ?? b.decapsCycles / 2500);
			default: {
				const av = String(a[col] ?? '').toLowerCase();
				const bv = String(b[col] ?? '').toLowerCase();
				return av < bv ? -1 : av > bv ? 1 : 0;
			}
		}
	}

	const sortedRows = $derived(
		[...rows].sort((a, b) => {
			const cmp = compare(a, b, sortCol);
			return sortDir === 'asc' ? cmp : -cmp;
		})
	);

	function setSort(col: KemSortableColumn) {
		if (sortCol === col) {
			sortDir = sortDir === 'asc' ? 'desc' : 'asc';
		} else {
			sortCol = col;
			sortDir = 'asc';
		}
	}

	function sortIndicator(col: KemSortableColumn): string {
		if (sortCol !== col) return '';
		return sortDir === 'asc' ? ' ▲' : ' ▼';
	}
</script>

{#snippet timeCell(us: number | null, cyc: number)}
	<td class="px-3 py-1.5 text-right tabular-nums">
		{#if us != null}
			{fmtTime(us)}
		{:else if cyc > 0}
			<span
				class="underline decoration-wavy decoration-pqs-scarlet"
				title="Estimated from {fmtCycles(cyc)} cycles @ 2.5 GHz"
				aria-label="{fmtTime(cyc / 2500)} (estimated from {fmtCycles(cyc)} cycles @ 2.5 GHz)"
			>{fmtTime(cyc / 2500)}</span>
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
						scope="col"
						class="cursor-pointer select-none whitespace-nowrap px-3 py-2.5 text-left font-semibold hover:bg-pqs-steel-light {col.numeric ? 'text-right' : ''}"
						onclick={() => setSort(col.key)}
					>
						{col.label}{sortIndicator(col.key)}
					</th>
				{/each}
				<th scope="col" class="px-3 py-2.5 text-left text-xs font-semibold">Assumption</th>
			</tr>
		</thead>
		<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
			{#each sortedRows as row (row.scheme + row.parameterset)}
				<tr class="hover:bg-pqs-smoke dark:hover:bg-pqs-steel/30">
					<!-- Scheme -->
					<td class="whitespace-nowrap px-3 py-1.5">
						<a
							href={row.website}
							target="_blank"
							rel="noopener noreferrer"
							class="font-heading font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot dark:hover:text-pqs-apricot-light"
						>
							{row.scheme}
						</a>
						<SecurityBadge
							broken={row.broken}
							warning={row.warning}
							info={row.info}
							classical={row.classical}
						/>
					</td>
					<!-- Category -->
					<td class="whitespace-nowrap px-3 py-1.5 text-pqs-steel dark:text-pqs-bluegray">
						{row.category}
					</td>
					<!-- Status -->
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
					<!-- Parameterset -->
					<td class="whitespace-nowrap px-3 py-1.5 font-mono text-xs">{row.parameterset}</td>
					<!-- Level -->
					<td class="px-3 py-1.5 text-right tabular-nums text-pqs-steel dark:text-pqs-bluegray">
						{row.level === 'Pre-Quantum' ? 'N/A' : row.level}
					</td>
					<!-- pk -->
					<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pk)}</td>
					<!-- ct -->
					<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.ct)}</td>
					<!-- pk+ct -->
					<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pkPlusCt)}</td>
					<!-- keygen / encaps / decaps -->
					{@render timeCell(row.keygenUs, row.keygenCycles)}
					{@render timeCell(row.encapsUs, row.encapsCycles)}
					{@render timeCell(row.decapsUs, row.decapsCycles)}
					<!-- Assumption -->
					<td class="px-3 py-1.5 text-pqs-steel/80 dark:text-pqs-bluegray">{row.assumption}</td>
				</tr>
			{/each}
			{#if sortedRows.length === 0}
				<tr>
					<td colspan="12" class="px-3 py-10 text-center text-pqs-bluegray">No KEMs to display.</td>
				</tr>
			{/if}
		</tbody>
	</table>
	<div class="border-t border-pqs-ashgray bg-pqs-smoke px-3 py-1.5 font-heading text-xs text-pqs-steel dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-bluegray">
		{sortedRows.length} parameter set{sortedRows.length === 1 ? '' : 's'}
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
