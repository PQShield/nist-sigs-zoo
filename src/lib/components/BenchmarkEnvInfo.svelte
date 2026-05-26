<script lang="ts">
	import type { BenchmarkEnv } from '$lib/types';

	let { env }: { env: BenchmarkEnv } = $props();

	const date = $derived(
		new Date(env.date).toLocaleDateString('en-GB', {
			year: 'numeric',
			month: 'long',
			day: 'numeric',
		}),
	);
</script>

<section
	class="rounded border border-pqs-ashgray bg-white p-4 text-xs text-pqs-midnight dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-smoke"
>
	<h2 class="mb-3 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
		Benchmark environment
	</h2>

	<div class="grid gap-x-8 gap-y-1 sm:grid-cols-2">
		<div class="flex gap-2">
			<span class="w-20 shrink-0 text-pqs-bluegray dark:text-pqs-bluegray">Date</span>
			<span>{date}</span>
		</div>
		<div class="flex gap-2">
			<span class="w-20 shrink-0 text-pqs-bluegray dark:text-pqs-bluegray">CPU</span>
			<span
				>{env.cpu.model} &mdash; {env.cpu.cores} cores,
				{env.cpu.threads_per_core} threads/core,
				{env.cpu.max_freq_mhz}&thinsp;MHz max, governor: {env.cpu.governor}, turbo: {env.cpu.turbo}</span
			>
		</div>
		<div class="flex gap-2">
			<span class="w-20 shrink-0 text-pqs-bluegray dark:text-pqs-bluegray">OS</span>
			<span>{env.os} (kernel {env.kernel})</span>
		</div>
		<div class="flex gap-2">
			<span class="w-20 shrink-0 text-pqs-bluegray dark:text-pqs-bluegray">Compiler</span>
			<span>{env.compiler}</span>
		</div>
		<div class="flex gap-2">
			<span class="w-20 shrink-0 text-pqs-bluegray dark:text-pqs-bluegray">OpenSSL</span>
			<span>{env.openssl}</span>
		</div>
		<div class="flex gap-2">
			<span class="w-20 shrink-0 text-pqs-bluegray dark:text-pqs-bluegray">Method</span>
			<span>{env.notes}</span>
		</div>
	</div>

	{#if env.sources && Object.keys(env.sources).length > 0}
		<details class="mt-3">
			<summary
				class="cursor-pointer select-none font-heading text-xs font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-bluegray dark:hover:text-pqs-apricot"
			>
				Implementation sources ▸
			</summary>
			<ul class="mt-2 space-y-0.5 font-mono">
				{#each Object.entries(env.sources).sort() as [scheme, url]}
					{@const atIdx = url.lastIndexOf('@')}
					{@const hasCommit = atIdx > 8}
					{@const repoUrl = hasCommit ? url.slice(0, atIdx) : url}
					{@const commit = hasCommit ? url.slice(atIdx + 1) : null}
					<li class="flex gap-2">
						<span class="w-16 shrink-0 text-pqs-steel dark:text-pqs-bluegray">{scheme}</span>
						<a
							href={commit ? `${repoUrl}/commit/${commit}` : repoUrl}
							target="_blank"
							rel="noopener noreferrer"
							class="truncate text-pqs-apricot hover:underline"
							title={url}
						>
							{#if commit}
								{repoUrl.replace('https://github.com/', '')}@{commit.slice(0, 8)}
							{:else}
								{url.replace('https://', '')}
							{/if}
						</a>
					</li>
				{/each}
			</ul>
		</details>
	{/if}

	<p class="mt-3 text-pqs-bluegray/80 dark:text-pqs-bluegray/60">
		Benchmark data licensed under
		<a
			href="https://creativecommons.org/licenses/by/4.0/"
			target="_blank"
			rel="noopener noreferrer"
			class="text-pqs-apricot hover:underline">{env.license}</a
		>
		&mdash; {env.attribution}
	</p>
</section>
