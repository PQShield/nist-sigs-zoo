<script lang="ts">
	import type { SortableColumn } from '$lib/types';
	import { getFilterStore } from '$lib/filterStore';
	import SecurityBadge from './SecurityBadge.svelte';

	const { store, filteredRows } = getFilterStore();

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

	const COLUMNS: { key: SortableColumn; label: string; numeric?: boolean }[] = [
		{ key: 'scheme', label: 'Scheme' },
		{ key: 'category', label: 'Category' },
		{ key: 'status', label: 'Status' },
		{ key: 'parameterset', label: 'Parameter Set' },
		{ key: 'level', label: 'Level', numeric: true },
		{ key: 'pk', label: 'pk (B)', numeric: true },
		{ key: 'sig', label: 'sig (B)', numeric: true },
		{ key: 'pkPlusSig', label: 'pk+sig (B)', numeric: true },
		{ key: 'signingUs', label: 'Sign', numeric: true },
		{ key: 'verificationUs', label: 'Verify', numeric: true },
	];

	function setSort(col: SortableColumn) {
		store.update((f) => ({
			...f,
			sortCol: col,
			sortDir: f.sortCol === col && f.sortDir === 'asc' ? 'desc' : 'asc',
		}));
	}

	function sortIndicator(col: SortableColumn): string {
		if ($store.sortCol !== col) return '';
		return $store.sortDir === 'asc' ? ' ▲' : ' ▼';
	}
</script>

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
				<th scope="col" class="px-3 py-2.5 text-left text-xs font-semibold">
					Assumption
				</th>
			</tr>
		</thead>
		<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
			{#each $filteredRows as row (row.scheme + row.parameterset)}
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
					<!-- sig -->
					<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.sig)}</td>
					<!-- pk+sig -->
					<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pkPlusSig)}</td>
					<!-- signing time -->
					<td class="px-3 py-1.5 text-right tabular-nums">
						{#if row.signingUs != null}
							{fmtTime(row.signingUs)}
						{:else if row.signingCycles > 0}
							<span
								class="underline decoration-wavy decoration-pqs-scarlet"
								title="Estimated from {fmtCycles(row.signingCycles)} cycles @ 2.5 GHz"
							>
								{fmtTime(row.signingCycles / 2500)}
							</span>
						{:else}
							<span class="text-pqs-bluegray">—</span>
						{/if}
					</td>
					<!-- verification time -->
					<td class="px-3 py-1.5 text-right tabular-nums">
						{#if row.verificationUs != null}
							{fmtTime(row.verificationUs)}
						{:else if row.verificationCycles > 0}
							<span
								class="underline decoration-wavy decoration-pqs-scarlet"
								title="Estimated from {fmtCycles(row.verificationCycles)} cycles @ 2.5 GHz"
							>
								{fmtTime(row.verificationCycles / 2500)}
							</span>
						{:else}
							<span class="text-pqs-bluegray">—</span>
						{/if}
					</td>
					<!-- Assumption -->
					<td class="px-3 py-1.5 text-pqs-steel/80 dark:text-pqs-bluegray">{row.assumption}</td>
				</tr>
			{/each}
			{#if $filteredRows.length === 0}
				<tr>
					<td colspan="11" class="px-3 py-10 text-center text-pqs-bluegray">
						No parameter sets match the current filters.
					</td>
				</tr>
			{/if}
		</tbody>
	</table>
	<div class="border-t border-pqs-ashgray bg-pqs-smoke px-3 py-1.5 font-heading text-xs text-pqs-steel dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-bluegray">
		{$filteredRows.length} parameter set{$filteredRows.length === 1 ? '' : 's'}
	</div>
</div>
