#include "StackBlur.h"

void StackBlur::Blur(Bitmap^ img, int radius)
{
	radius = Math::Min(254, radius);

	if (radius < 1) return;
	radius |= 0;


	BitmapData^ bitmapData = img->LockBits(Rectangle(0, 0, img->Width, img->Height), ImageLockMode::ReadOnly, img->PixelFormat);
	IntPtr ptr = bitmapData->Scan0;
	int bytes = Math::Abs(bitmapData->Stride) * img->Height;
	array<Byte>^ rgb = gcnew array<Byte>(bytes);
	System::Runtime::InteropServices::Marshal::Copy(ptr, rgb, 0, bytes);
	array<int>^ pixels = gcnew array<int>(rgb->Length);
	int r,g,b;
	for (int i = 0; i < rgb->Length; i += 4)
	{
		if (rgb[i + 3] > 0) {
			r = rgb[i + 0];
			g = rgb[i + 1];
			b = rgb[i + 2];
			break;
		}
	}
	for (int i = 0; i < rgb->Length; i += 4)
	{
		if (rgb[i + 3] > 0) {
			r = rgb[i + 0];
			g = rgb[i + 1];
			b = rgb[i + 2];
		}
		pixels[i + 0] = b;
		pixels[i + 1] = g;
		pixels[i + 2] = r;
		pixels[i + 3] = rgb[i + 3];
	}
	img->UnlockBits(bitmapData);

	int x, y, i, p, yp, yi, yw, r_sum, g_sum, b_sum, a_sum,
	r_out_sum, g_out_sum, b_out_sum, a_out_sum,
	r_in_sum, g_in_sum, b_in_sum, a_in_sum,
	pr, pg, pb, pa, rbs;

	int div = radius + radius + 1;
	int w4 = img->Width << 2;
	int widthMinus1 = img->Width - 1;
	int heightMinus1 = img->Height - 1;
	int radiusPlus1 = radius + 1;
	int sumFactor = radiusPlus1 * (radiusPlus1 + 1) / 2;

	BlurStack^ stackStart = gcnew BlurStack();
	BlurStack^ stack = stackStart;
	BlurStack^ stackEnd = null;
	for (i = 1; i < div; i++)
	{
		stack = stack->next = gcnew BlurStack();
		if (i == radiusPlus1)
			stackEnd = stack;
	}
	stack->next = stackStart;
	BlurStack^ stackIn = null;
	BlurStack^ stackOut = null;

	yw = yi = 0;

	int mul_sum = mul_table[radius];
	int shg_sum = shg_table[radius];

	for (y = 0; y < img->Height; y++)
	{
		r_in_sum = g_in_sum = b_in_sum = a_in_sum = r_sum = g_sum = b_sum = a_sum = 0;

		r_out_sum = radiusPlus1 * (pr = pixels[yi]);
		g_out_sum = radiusPlus1 * (pg = pixels[yi + 1]);
		b_out_sum = radiusPlus1 * (pb = pixels[yi + 2]);
		a_out_sum = radiusPlus1 * (pa = pixels[yi + 3]);

		r_sum += sumFactor * pr;
		g_sum += sumFactor * pg;
		b_sum += sumFactor * pb;
		a_sum += sumFactor * pa;

		stack = stackStart;

		for (i = 0; i < radiusPlus1; i++)
		{
			stack->r = pr;
			stack->g = pg;
			stack->b = pb;
			stack->a = pa;
			stack = stack->next;
		}

		for (i = 1; i < radiusPlus1; i++)
		{
			p = yi + ((widthMinus1 < i ? widthMinus1 : i) << 2);
			r_sum += (stack->r = (pr = pixels[p])) * (rbs = radiusPlus1 - i);
			g_sum += (stack->g = (pg = pixels[p + 1])) * rbs;
			b_sum += (stack->b = (pb = pixels[p + 2])) * rbs;
			a_sum += (stack->a = (pa = pixels[p + 3])) * rbs;

			r_in_sum += pr;
			g_in_sum += pg;
			b_in_sum += pb;
			a_in_sum += pa;

			stack = stack->next;
		}


		stackIn = stackStart;
		stackOut = stackEnd;
		for (x = 0; x < img->Width; x++)
		{
			pixels[yi + 3] = pa = (a_sum * mul_sum) >> shg_sum;
			pixels[yi] = ((r_sum * mul_sum) >> shg_sum);
			pixels[yi + 1] = ((g_sum * mul_sum) >> shg_sum);
			pixels[yi + 2] = ((b_sum * mul_sum) >> shg_sum);

			//It was removed due to a color problem

			/*if (pa != 0)
			{
				pa = 255 / pa;
				pixels[yi] = Math::Min(255, ((r_sum * mul_sum) >> shg_sum) * pa);
				pixels[yi + 1] = Math::Min(255, ((g_sum * mul_sum) >> shg_sum) * pa);
				pixels[yi + 2] = Math::Min(255, ((b_sum * mul_sum) >> shg_sum) * pa);
			}
			else
			{
				pixels[yi] = pixels[yi + 1] = pixels[yi + 2] = 0;
			}*/

			r_sum -= r_out_sum;
			g_sum -= g_out_sum;
			b_sum -= b_out_sum;
			a_sum -= a_out_sum;

			r_out_sum -= stackIn->r;
			g_out_sum -= stackIn->g;
			b_out_sum -= stackIn->b;
			a_out_sum -= stackIn->a;

			p = (yw + ((p = x + radius + 1) < widthMinus1 ? p : widthMinus1)) << 2;

			r_in_sum += (stackIn->r = pixels[p]);
			g_in_sum += (stackIn->g = pixels[p + 1]);
			b_in_sum += (stackIn->b = pixels[p + 2]);
			a_in_sum += (stackIn->a = pixels[p + 3]);

			r_sum += r_in_sum;
			g_sum += g_in_sum;
			b_sum += b_in_sum;
			a_sum += a_in_sum;

			stackIn = stackIn->next;

			r_out_sum += (pr = stackOut->r);
			g_out_sum += (pg = stackOut->g);
			b_out_sum += (pb = stackOut->b);
			a_out_sum += (pa = stackOut->a);

			r_in_sum -= pr;
			g_in_sum -= pg;
			b_in_sum -= pb;
			a_in_sum -= pa;

			stackOut = stackOut->next;

			yi += 4;
		}
		yw += img->Width;
	}


	for (x = 0; x < img->Width; x++)
	{
		g_in_sum = b_in_sum = a_in_sum = r_in_sum = g_sum = b_sum = a_sum = r_sum = 0;

		yi = x << 2;
		r_out_sum = radiusPlus1 * (pr = pixels[yi]);
		g_out_sum = radiusPlus1 * (pg = pixels[yi + 1]);
		b_out_sum = radiusPlus1 * (pb = pixels[yi + 2]);
		a_out_sum = radiusPlus1 * (pa = pixels[yi + 3]);

		r_sum += sumFactor * pr;
		g_sum += sumFactor * pg;
		b_sum += sumFactor * pb;
		a_sum += sumFactor * pa;

		stack = stackStart;

		for (i = 0; i < radiusPlus1; i++)
		{
			stack->r = pr;
			stack->g = pg;
			stack->b = pb;
			stack->a = pa;
			stack = stack->next;
		}

		yp = img->Width;

		for (i = 1; i <= radius; i++)
		{
			yi = (yp + x) << 2;

			r_sum += (stack->r = (pr = pixels[yi])) * (rbs = radiusPlus1 - i);
			g_sum += (stack->g = (pg = pixels[yi + 1])) * rbs;
			b_sum += (stack->b = (pb = pixels[yi + 2])) * rbs;
			a_sum += (stack->a = (pa = pixels[yi + 3])) * rbs;

			r_in_sum += pr;
			g_in_sum += pg;
			b_in_sum += pb;
			a_in_sum += pa;

			stack = stack->next;

			if (i < heightMinus1)
			{
				yp += img->Width;
			}
		}

		yi = x;
		stackIn = stackStart;
		stackOut = stackEnd;
		for (y = 0; y < img->Height; y++)
		{
			p = yi << 2;
			pixels[p + 3] = pa = (a_sum * mul_sum) >> shg_sum;
			pixels[p] = ((r_sum * mul_sum) >> shg_sum);
			pixels[p + 1] = ((g_sum * mul_sum) >> shg_sum);
			pixels[p + 2] = ((b_sum * mul_sum) >> shg_sum);

			//It was removed due to a color problem

			/*if (pa > 0)
			{
				pa = 255 / pa;
				pixels[p] = Math::Min(255,((r_sum * mul_sum) >> shg_sum) * pa);
				pixels[p + 1] = Math::Min(255, ((g_sum * mul_sum) >> shg_sum) * pa);
				pixels[p + 2] = Math::Min(255, ((b_sum * mul_sum) >> shg_sum) * pa);
			}
			else
			{
				pixels[p] = pixels[p + 1] = pixels[p + 2] = 0;
			}*/

			r_sum -= r_out_sum;
			g_sum -= g_out_sum;
			b_sum -= b_out_sum;
			a_sum -= a_out_sum;

			r_out_sum -= stackIn->r;
			g_out_sum -= stackIn->g;
			b_out_sum -= stackIn->b;
			a_out_sum -= stackIn->a;

			p = (x + (((p = y + radiusPlus1) < heightMinus1 ? p : heightMinus1) * img->Width)) << 2;

			r_sum += (r_in_sum += (stackIn->r = pixels[p]));
			g_sum += (g_in_sum += (stackIn->g = pixels[p + 1]));
			b_sum += (b_in_sum += (stackIn->b = pixels[p + 2]));
			a_sum += (a_in_sum += (stackIn->a = pixels[p + 3]));

			stackIn = stackIn->next;

			r_out_sum += (pr = stackOut->r);
			g_out_sum += (pg = stackOut->g);
			b_out_sum += (pb = stackOut->b);
			a_out_sum += (pa = stackOut->a);

			r_in_sum -= pr;
			g_in_sum -= pg;
			b_in_sum -= pb;
			a_in_sum -= pa;

			stackOut = stackOut->next;

			yi += img->Width;
		}
	}

	bitmapData = img->LockBits(Rectangle(0, 0, img->Width, img->Height), ImageLockMode::ReadWrite, img->PixelFormat);
	ptr = bitmapData->Scan0;
	bytes = Math::Abs(bitmapData->Stride) * img->Height;
	rgb = gcnew array<Byte>(bytes);
	System::Runtime::InteropServices::Marshal::Copy(ptr, rgb, 0, bytes);
	for (int i = 0; i < rgb->Length; i += 4)
	{
		rgb[i + 0] = pixels[i + 2];
		rgb[i + 1] = pixels[i + 1];
		rgb[i + 2] = pixels[i + 0];
		rgb[i + 3] = pixels[i + 3];
	}
	System::Runtime::InteropServices::Marshal::Copy(rgb, 0, ptr, bytes);
	img->UnlockBits(bitmapData);
}
