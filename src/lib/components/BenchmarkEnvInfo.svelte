<script lang="ts">
	import { TIMING_VARIABILITY_NOTE } from '$lib/constants';
	import type { BenchmarkEnv, BenchmarkEnvFile } from '$lib/types';

	/** Every environment in the file: each one some parameter set's timings come from. */
	let { file }: { file: BenchmarkEnvFile } = $props();

	const longDate = (env: BenchmarkEnv) =>
		new Date(env.date).toLocaleDateString('en-GB', {
			year: 'numeric',
			month: 'long',
			day: 'numeric'
		});
	const many = $derived(file.environments.length > 1);
</script>

<section
	class="rounded border border-pqs-ashgray bg-white p-4 text-xs text-pqs-midnight dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-smoke"
>
	<h2 class="mb-3 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
		Benchmark environment{many ? `s (${file.environments.length})` : ''}
	</h2>

	{#if many}
		<p class="mb-3">
			Timings come from more than one run; each parameter set's timing cells name their run in the tooltip on its scheme
			page.
		</p>
	{/if}
	<p class="mb-3">{TIMING_VARIABILITY_NOTE}</p>
	{#each file.environments as env (env.id)}
		<div class="mt-3 first-of-type:mt-0" data-env={env.id}>
			{#if many}
				<h3 class="mb-1 font-heading text-xs font-semibold text-pqs-steel dark:text-pqs-bluegray">{env.id}</h3>
			{/if}
			<div class="grid gap-x-8 gap-y-1 sm:grid-cols-2">
				<div class="flex gap-2">
					<span class="w-20 shrink-0 text-pqs-steel dark:text-pqs-bluegray">Date</span>
					<span>{longDate(env)}</span>
				</div>
				{#if env.instance_type}
					<div class="flex gap-2">
						<span class="w-20 shrink-0 text-pqs-steel dark:text-pqs-bluegray">Machine</span>
						<span>{env.instance_type === 'local' ? 'local machine' : `EC2 ${env.instance_type}`}</span>
					</div>
				{/if}
				<div class="flex gap-2">
					<span class="w-20 shrink-0 text-pqs-steel dark:text-pqs-bluegray">CPU</span>
					<span
						>{env.cpu.model} &mdash; {env.cpu.cores} cores,
						{env.cpu.threads_per_core} threads/core,
						{env.cpu.max_freq_mhz}&thinsp;MHz max, governor: {env.cpu.governor}, turbo: {env.cpu.turbo}</span
					>
				</div>
				<div class="flex gap-2">
					<span class="w-20 shrink-0 text-pqs-steel dark:text-pqs-bluegray">OS</span>
					<span>{env.os} (kernel {env.kernel})</span>
				</div>
				<div class="flex gap-2">
					<span class="w-20 shrink-0 text-pqs-steel dark:text-pqs-bluegray">Compiler</span>
					<span>{env.compiler}</span>
				</div>
				<div class="flex gap-2">
					<span class="w-20 shrink-0 text-pqs-steel dark:text-pqs-bluegray">OpenSSL</span>
					<span>{env.openssl}</span>
				</div>
				<div class="flex gap-2">
					<span class="w-20 shrink-0 text-pqs-steel dark:text-pqs-bluegray">Method</span>
					<span>{env.notes}</span>
				</div>
			</div>
			{#if env.sources && Object.keys(env.sources).length > 0}
				<details class="mt-3">
					<summary
						class="cursor-pointer font-heading text-xs font-semibold text-pqs-steel select-none hover:text-pqs-apricot dark:text-pqs-bluegray dark:hover:text-pqs-apricot"
					>
						Implementation sources ▸
					</summary>
					<ul class="mt-2 space-y-0.5 font-mono">
						{#each Object.entries(env.sources).sort() as [scheme, url] (scheme)}
							{@const m = url.match(/^(.*)@([0-9a-f]{7,40})$/)}
							{@const repoUrl = (m ? m[1] : url).replace(/\.git$/, '')}
							{@const commit = m ? m[2] : null}
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
									<span class="sr-only"> (opens in a new tab)</span>
								</a>
							</li>
						{/each}
					</ul>
				</details>
			{/if}
		</div>
	{/each}

	<p class="mt-3 text-pqs-steel/70 dark:text-pqs-bluegray/70">
		Benchmark data licensed under
		<a
			href="https://creativecommons.org/licenses/by/4.0/"
			target="_blank"
			rel="noopener noreferrer"
			class="text-pqs-apricot hover:underline">{file.license}<span class="sr-only"> (opens in a new tab)</span></a
		>
		&mdash; {file.attribution}
	</p>
</section>
